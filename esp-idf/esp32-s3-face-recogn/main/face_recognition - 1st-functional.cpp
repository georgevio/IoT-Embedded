/**
 * @file face_recognition.cpp
 * @brief ESP-WHO Face Recognition app minimal
 */

#include <stdio.h>
#include <list>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"

#include "dl_image_define.hpp"
#include "dl_detect_define.hpp"
#include "dl_recognition_define.hpp"
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp"

// Image resolution. this is for QQVGA resolution (160x120) from ESP32-CAM. Can be altered
#define IMAGE_WIDTH 160
#define IMAGE_HEIGHT 120

static const char *TAG = "FACE_REC_APP";
static QueueHandle_t recognition_result_queue = NULL;

/**
 * @brief face recognition on the incoming image buffer.
 * 
 * @param image_buffer Pointer to the image data buffer.
 */
static void perform_face_recognition(uint8_t *image_buffer)
{
    // Image Container. Can add other metadata, e.g., date/time, quality of image, etc.
    dl::image::img_t image;
    image.width = IMAGE_WIDTH;
    image.height = IMAGE_HEIGHT;
    image.data = image_buffer;
    image.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

    // detector and recognizer objects. From esp-who libraries, BE CAREFUL with versions
    HumanFaceDetect *detector = new HumanFaceDetect();
    HumanFaceRecognizer *recognizer = new HumanFaceRecognizer(nullptr);

    int face_id = -1;

    // Face Detection
    std::list<dl::detect::result_t> faces = detector->run(image);

    // Face Recognition
    if (faces.empty())
    {
        ESP_LOGI(TAG, "No face detected in the frame.");
    }
    else
    {
        std::vector<dl::recognition::result_t> results = recognizer->recognize(image, faces);
        if (results.empty())
        {
            ESP_LOGI(TAG, "Recognition result: unknown face");
            face_id = -1;
        }
        else
        {
            face_id = results.front().id;
            ESP_LOGI(TAG, "Recognition found face ID: %d, with similarity: %f", face_id, results.front().similarity);
        }
    }

    delete detector;
    delete recognizer;

    // Send result to the queue
    xQueueSend(recognition_result_queue, &face_id, portMAX_DELAY);
}

/**
 * @brief Simulate image feeding and process the results.
 */
static void image_feeder_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Image feeder started.");
    uint8_t *image_data = (uint8_t *)heap_caps_malloc(IMAGE_WIDTH * IMAGE_HEIGHT * 3, MALLOC_CAP_SPIRAM);
    if (!image_data) {
        ESP_LOGE(TAG, "Could not allocate memory for image data!");
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Simulating image capture...");
        memset(image_data, 128, IMAGE_WIDTH * IMAGE_HEIGHT * 3);

        perform_face_recognition(image_data);

        int recognized_id = -1;
        if (xQueueReceive(recognition_result_queue, &recognized_id, pdMS_TO_TICKS(5000))) {
            ESP_LOGI(TAG, "Result face ID %d.", recognized_id);
            if (recognized_id >= 0) {
                ESP_LOGW(TAG, "Face %d", recognized_id);
            } else {
                ESP_LOGW(TAG, "Unknown face.");
            }
        } else {
            ESP_LOGE(TAG, "No face detected in image.");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    heap_caps_free(image_data);
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP-WHO Face Recognition Start...");
    
    recognition_result_queue = xQueueCreate(1, sizeof(int));
    if(recognition_result_queue == NULL){
        ESP_LOGE(TAG, "Error creating result queue");
        return;
    }

    xTaskCreate(image_feeder_task, "image_feeder_task", 8 * 1024, NULL, 5, NULL);
}
