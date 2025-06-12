// ABANDONEND big whole frame of 150KB could not be handled by the server


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
// malloc, free, memcpy 
#include <stdlib.h>
#include <string.h>

#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "wifi.h"                   
#include "websocket_client.h"     

static EventGroupHandle_t s_app_event_group;
const static int WIFI_CONNECTED_BIT = (1 << 0);
const static int WEBSOCKET_CONNECTED_BIT = (1 << 1); 
const static int FRAME_ACK_BIT = (1 << 2);          
const static int FRAME_QUEUE_SIZE = 2;              

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueFaceFrame = NULL;
static const char* TAG_APP_MAIN = "MAIN_APP";

#define HEARTBEAT_INTERVAL_S 90 
#define HEARTBEAT_ON 1 
#define SERVER_ACK_TIMEOUT_MS 30000 
// Stop the camera after a face detection to stop errors while trying to send
#define POST_DETECTION_COOLDOWN_S 120 // started with 5 sec 


#if HEARTBEAT_ON 
static void heartbeat_task(void* pvParameters) {
    while(true) {
        xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_S * 1000));
        // Terminal messages in websocket_client.cpp
        websocket_send_heartbeat(); 
    }
}
#endif

// will also stop the camera while sending
static void face_sending_task(void* pvParameters) {
    camera_fb_t* frame = NULL;
    while (true) {
        if (xQueueReceive(xQueueFaceFrame, &frame, portMAX_DELAY)) {
            if (!frame || !frame->buf) {
                if (frame) {
                    esp_camera_fb_return(frame);
                }
                continue;
            }

            // Stop camera capture to avoid buffer problems
            ESP_LOGI(TAG_APP_MAIN, "Face detected. Stopping camera temporaly.");
            camera_stop();

            // --- MODIFICATION: Flush queues to prevent back-to-back detections ---
            ESP_LOGI(TAG_APP_MAIN, "Flushing processing queues.");
            xQueueReset(xQueueAIFrame);
            xQueueReset(xQueueFaceFrame);
            // --- END MODIFICATION ---

            ESP_LOGI(TAG_APP_MAIN, "Copying frame (size: %zu).", frame->len);
            
            uint8_t *frame_copy = (uint8_t *)malloc(frame->len);
            if (!frame_copy) {
                ESP_LOGE(TAG_APP_MAIN, "FATAL: Failed to allocate %zu bytes for frame copy!", frame->len);
                esp_camera_fb_return(frame);
                // Restart camera no matter what
                camera_start(); 
                continue;
            }

            size_t frame_len = frame->len;
            memcpy(frame_copy, frame->buf, frame_len);

            esp_camera_fb_return(frame);
            ESP_LOGI(TAG_APP_MAIN, "Original buffer released.");
            
            ESP_LOGI(TAG_APP_MAIN, "Waiting for network...");
            xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
            
            xEventGroupClearBits(s_app_event_group, FRAME_ACK_BIT);

            ESP_LOGI(TAG_APP_MAIN, "Network ready. Sending copied frame.");
            
            if (websocket_send_frame(frame_copy, frame_len) == ESP_OK) {
                EventBits_t bits = xEventGroupWaitBits(s_app_event_group, FRAME_ACK_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(SERVER_ACK_TIMEOUT_MS));
                
                if (bits & FRAME_ACK_BIT) {
                    ESP_LOGI(TAG_APP_MAIN, "Server acknowledged frame!");
                } else {
                    ESP_LOGE(TAG_APP_MAIN, "Frame sent, but no ACK within %d ms.", SERVER_ACK_TIMEOUT_MS);
                }
            } else {
                ESP_LOGE(TAG_APP_MAIN, "Failed to send frame.");
            }
            
            free(frame_copy);

            ESP_LOGI(TAG_APP_MAIN, "Halt camera for %d-sec.", POST_DETECTION_COOLDOWN_S);
            vTaskDelay(pdMS_TO_TICKS(POST_DETECTION_COOLDOWN_S * 1000));
            ESP_LOGI(TAG_APP_MAIN, "Restarting camera.");
            camera_start();
        }
    }
}

static void app_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG_APP_MAIN, "WiFi started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG_APP_MAIN, "WiFi disconnected.");
        xEventGroupClearBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT);
        websocket_client_stop();
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG_APP_MAIN, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_app_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG_APP_MAIN, "WiFi connected. Starting WebSocket client.");
        websocket_client_start(s_app_event_group);
    }
}

extern "C" void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_app_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &app_event_handler, NULL));
    
    wifi_init_sta();

    xQueueAIFrame = xQueueCreate(FRAME_QUEUE_SIZE, sizeof(camera_fb_t*));
    xQueueFaceFrame = xQueueCreate(FRAME_QUEUE_SIZE, sizeof(camera_fb_t*));
    
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);

    register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueFaceFrame, false);
    
    xTaskCreate(face_sending_task, "face_sender_task", 4096, NULL, 5, NULL);
#if HEARTBEAT_ON
    ESP_LOGI(TAG_APP_MAIN, "Ping on, every %d sec.", HEARTBEAT_INTERVAL_S);
    xTaskCreate(heartbeat_task, "heartbeat_task", 3072, NULL, 5, NULL);
#endif
    ESP_LOGI(TAG_APP_MAIN, "App started all workers.");
}