
// #############################################################################
// #                                                                           #
// #    NOTICE: THIS FILE HAS BEEN MODIFIED FROM THE ORIGINAL ESP-WHO VERSION  #
// #                                                                           #
// #    Last Modified by: George                                               #
// #    Last Modification Date: May 31, 2025                                   #
// #                                                                           #
// #    Changes:                                                               #
// #      - Added 'print_detection_result' for detailed console output         #
// #        of face detection results.                                         #
// #      - Uncommented the call to 'print_detection_result'.                  #
// #      - Included <list> explicitly.                                        #
// #      - Minor adjustments like printf statements for debugging and         #
// #        task creation error checks.                                        #
// #                                                                           #
// #############################################################################

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "who_human_face_detection.hpp"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"

// George
#include "who_ai_utils.hpp" // draw_detection_result
#include <stdio.h> // For printf
#include <list>    // For std::list

#define TWO_STAGE_ON 1 // George: More computationally intensive, and more accurate

static const char* TAG = "human_face_detection";

static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueEvent = NULL;
static QueueHandle_t xQueueFrameO = NULL;
static QueueHandle_t xQueueResult = NULL;

static bool gEvent = true;
static bool gReturnFB = true;

// +++ George: 
// print the details of detected faces to the console.
//void print_detection_result(const std::list<dl::detect::result_t>& results)
void print_detection_result(std::list<dl::detect::result_t>& results)
{
    printf("----------------------------------------------------------------\n");
    if (results.empty())
    {
        printf("DEBUG HFD_PRINT: No faces detected in this frame.\n");
    }
    else
    {
        printf("DEBUG HFD_PRINT: Found %zu face(s) in this frame:\n", results.size());
        int i = 0;
        for (const auto& result : results)
        {
            printf("  Face %d:\n", ++i);
            printf("    Score:    %.2f\n", result.score);
            printf("    Category: %d\n", result.category); // For faces, this is usually a specific ID
            printf("    Box:      [x=%d, y=%d, w=%d, h=%d]\n", result.box[0], result.box[1], result.box[2], result.box[3]);
            // print keypoints if they are used and available:
            // printf("    Keypoints: [");
            // for(int j=0; j<10; j+=2){ // Assuming 5 keypoints (x,y pairs)
            //     printf("(%d,%d) ", result.keypoint[j], result.keypoint[j+1]);
            // }
            // printf("]\n");
        }
    }
    printf("----------------------------------------------------------------\n");
}
// +++ End of print_detection_result +++

 void task_process_handler(void* arg)
{
    //George DEBUGGING, TOO MANY MESSAGES
    //printf("DEBUG HFD: task_process_handler started on Core %d!\n", xPortGetCoreID());
    camera_fb_t* frame = NULL;
    //George ORIGINAL VALUES
    //HumanFaceDetectMSR01 detector(0.3F, 0.3F, 10, 0.3F);
    HumanFaceDetectMSR01 detector(0.25F, 0.3F, 10, 0.3F);
#if TWO_STAGE_ON
    //George ORIGINAL VALUES
    //HumanFaceDetectMNP01 detector2(0.4F, 0.3F, 10); es
    HumanFaceDetectMNP01 detector2(0.35F, 0.3F, 10);
#endif

    while (true)
    {
        if (gEvent)
        {
            if (xQueueReceive(xQueueFrameI, &frame, portMAX_DELAY))
            {
                bool is_detected = false; // Moved declaration 
                //George TOO MANY MESSAGES
                //printf("DEBUG HFD: Frame received. Addr: %p, Len: %zu, W: %d, H: %d, Format: %d\n",
                // (void*)frame->buf, frame->len, frame->width, frame->height, frame->format);
#if TWO_STAGE_ON
                std::list<dl::detect::result_t>& detect_candidates = detector.infer((uint16_t*)frame->buf, { (int)frame->height, (int)frame->width, 3 });
                //George TOO MANY MESSAGES
                //printf("DEBUG HFD: Stage 1 detections: %zu\n", detect_candidates.size());
                std::list<dl::detect::result_t>& detect_results = detector2.infer((uint16_t*)frame->buf, { (int)frame->height, (int)frame->width, 3 }, detect_candidates);
#else
                std::list<dl::detect::result_t>& detect_results = detector.infer((uint16_t*)frame->buf, { (int)frame->height, (int)frame->width, 3 });
#endif
                //George TOO MANY MESSAGES
                //printf("DEBUG HFD: Final detections (Stage 2 or Single Stage): %zu\n", detect_results.size());

                if (detect_results.size() > 0)
                {
                    // George: comment to NOT print detection results in terminal (as in the original)
                    printf("DEBUG HFD: Faces DETECTED! Attempting to print results...\n");       
                    print_detection_result(detect_results);

                    draw_detection_result((uint16_t*)frame->buf, frame->height, frame->width, detect_results);
                    is_detected = true;
                }
				// else // Optional: print if no faces detected in this specific frame. Too many messages
                // {
                //    print_detection_result(detect_results); // print "No faces detected"
                // }


                if (xQueueFrameO)
                {
                    xQueueSend(xQueueFrameO, &frame, portMAX_DELAY);
                }
                else if (gReturnFB)
                {
                    esp_camera_fb_return(frame);
                }
                else
                {
                    free(frame);
                }
                //George
                frame = NULL; // Good practice
                if (xQueueResult)
                {
                    xQueueSend(xQueueResult, &is_detected, portMAX_DELAY);
                }
            } // End if xQueueReceive
        }     // End if gEvent
        vTaskDelay(pdMS_TO_TICKS(10)); 
    } // End while(true)
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
    const QueueHandle_t frame_o,
    const bool camera_fb_return)
{
    xQueueFrameI = frame_i;
    xQueueFrameO = frame_o;
    xQueueEvent = event;
    xQueueResult = result;
    gReturnFB = camera_fb_return;

    // It's good practice to check the return value of xTaskCreatePinnedToCore
    BaseType_t res_process = xTaskCreatePinnedToCore(task_process_handler, TAG, 4 * 1024, NULL, 5, NULL, 0);
    if (res_process != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task_process_handler. Error code: %d", res_process);
    }

    if (xQueueEvent) {
        BaseType_t res_event = xTaskCreatePinnedToCore(task_event_handler, TAG, 4 * 1024, NULL, 5, NULL, 1);
        if (res_event != pdPASS) {
            ESP_LOGE(TAG, "Failed to create task_event_handler. Error code: %d", res_event);
        }
    }
}