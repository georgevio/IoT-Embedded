/*                                                                               
 * IMPORTANT: THIS FILE HAS BEEN HEAVILY MODIFIED FROM THE ORIGINAL VERSION      
 *                                                                               
 * // George, May-June, 2025                                       
 *                                                                               
 * Changes:                                                                   
 * - Added 'print_detection_result' for printf in console                   
 *   of face detection results.                                             
 * - Call to 'print_detection_result' exists. Can be uncommented.                      
 * - Added several printf for debugging and                                 
 *   task creation error checks.                                            
 * - Changed task_process_handler to handle frame buffer 
 *   and only send frames with faces.                
 * - The 'task_process_handler' copies the bounding box coordinates 
 *   from the AI library struct into a struct here
 */

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "who_human_face_detection.hpp"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"

#include <stdio.h>
#include <list>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TWO_STAGE_ON 1
static const char* TAG = "human_face_detection";

static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueEvent = NULL;
static QueueHandle_t xQueueFrameO = NULL;
static QueueHandle_t xQueueResult = NULL;

static bool gEvent = true;

void task_process_handler(void* arg)
{
    camera_fb_t* frame = NULL;
    HumanFaceDetectMSR01 detector(0.25F, 0.3F, 10, 0.3F);
#if TWO_STAGE_ON
    HumanFaceDetectMNP01 detector2(0.35F, 0.3F, 10);
#endif

    while (true)
    {
        if (gEvent)
        {
            if (xQueueReceive(xQueueFrameI, &frame, portMAX_DELAY))
            {
                bool is_detected = false;
#if TWO_STAGE_ON
                std::list<dl::detect::result_t>& detect_candidates = detector.infer((uint16_t*)frame->buf, { (int)frame->height, (int)frame->width, 3 });
                std::list<dl::detect::result_t>& detect_results = detector2.infer((uint16_t*)frame->buf, { (int)frame->height, (int)frame->width, 3 }, detect_candidates);
#else
                std::list<dl::detect::result_t>& detect_results = detector.infer((uint16_t*)frame->buf, { (int)frame->height, (int)frame->width, 3 });
#endif

                if (detect_results.size() > 0)
                {
                    is_detected = true;
                    ESP_LOGI(TAG, "Face DETECTED!");       
                    
                    if (xQueueFrameO)
                    {
                        face_to_send_t *face_data = (face_to_send_t *)malloc(sizeof(face_to_send_t));
                        if(face_data) 
                        {
                            dl::detect::result_t first_face = detect_results.front();
                            
                            // George transfer coordinates from the library to a custom struct
                            face_data->fb = frame;
                            face_data->box.x = first_face.box[0];
                            face_data->box.y = first_face.box[1];
                            face_data->box.w = first_face.box[2];
                            face_data->box.h = first_face.box[3];

                            if (xQueueSend(xQueueFrameO, &face_data, 0) != pdTRUE)
                            {
                                ESP_LOGW(TAG, "Output frame queue is full. Dropping frame.");
                                esp_camera_fb_return(frame);
                                free(face_data);
                            }
                        } 
                        else 
                        {
                             ESP_LOGE(TAG, "Failed to allocate memory for face_data struct.");
                             esp_camera_fb_return(frame);
                        }
                    }
                    else
                    {
                        esp_camera_fb_return(frame);
                    }
                }
                else
                {
                    esp_camera_fb_return(frame);
                }
                
                frame = NULL; 

                if (xQueueResult)
                {
                    xQueueSend(xQueueResult, &is_detected, portMAX_DELAY);
                }
            }
        }    
        vTaskDelay(pdMS_TO_TICKS(10)); 
    } 
}

static void task_event_handler(void* arg)
{
    while (true)
    {
        xQueueReceive(xQueueEvent, &(gEvent), portMAX_DELAY);
    }
}

void register_human_face_detection(const QueueHandle_t frame_i,
    const QueueHandle_t event,
    const QueueHandle_t result,
    const QueueHandle_t frame_o)
{
    xQueueFrameI = frame_i;
    xQueueFrameO = frame_o;
    xQueueEvent = event;
    xQueueResult = result;

    xTaskCreatePinnedToCore(task_process_handler, TAG, 4 * 1024, NULL, 5, NULL, 0);

    if (xQueueEvent) {
        xTaskCreatePinnedToCore(task_event_handler, TAG, 4 * 1024, NULL, 5, NULL, 1);
    }
}