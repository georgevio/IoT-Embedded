#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <stdlib.h>
#include <string.h>

#include "who_camera.h"
#include "who_human_face_detection.hpp" // George added custom struct for the image
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

#define HEARTBEAT_INTERVAL_S 300 // Started with 30 sec. Is it needed? What about 5 min?
#define HEARTBEAT_ON 1
#define SERVER_ACK_TIMEOUT_MS 30
#define POST_DETECTION_COOLDOWN_S 30

#if HEARTBEAT_ON
static void heartbeat_task(void* pvParameters) {
    while(true) {
        xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_S * 1000));
        websocket_send_heartbeat();
    }
}
#endif

static void face_sending_task(void* pvParameters) {
    face_to_send_t *face_data = NULL;
    const size_t CHUNK_SIZE = 8192;

    while (true) {
        if (xQueueReceive(xQueueFaceFrame, &face_data, portMAX_DELAY)) {
            if (!face_data || !face_data->fb) {
                if (face_data) free(face_data);
                continue;
            }

            camera_fb_t* full_frame = face_data->fb;

            ESP_LOGI(TAG_APP_MAIN, "Face detected in frame %d. Stopping camera.", (int)face_data->id);
            camera_stop();

            ESP_LOGI(TAG_APP_MAIN, "Flushing processing queues.");
            xQueueReset(xQueueAIFrame);
            xQueueReset(xQueueFaceFrame);
            
            uint8_t *cropped_buf = NULL;
            size_t cropped_len = 0;

            do {
                int x = face_data->box.x;
                int y = face_data->box.y;
                int w = face_data->box.w;
                int h = face_data->box.h;
                uint32_t frame_id = face_data->id;

                if (x < 0) { x = 0; }
                if (y < 0) { y = 0; }
                if (x + w > (int)full_frame->width) { w = full_frame->width - x; }
                if (y + h > (int)full_frame->height) { h = full_frame->height - y; }
                if (w <= 0 || h <= 0) {
                    ESP_LOGE(TAG_APP_MAIN, "Invalid crop dimensions for frame %d", (int)frame_id);
                    break;
                }

                cropped_len = w * h * 2;
                cropped_buf = (uint8_t *)malloc(cropped_len);
                if (!cropped_buf) {
                    ESP_LOGE(TAG_APP_MAIN, "Failed to allocate memory for cropped frame!");
                    break;
                }

                uint16_t *p_full = (uint16_t *)full_frame->buf;
                uint16_t *p_cropped = (uint16_t *)cropped_buf;
                for (int row = 0; row < h; ++row) {
                    memcpy(
                        p_cropped + (row * w),
                        p_full + ((y + row) * full_frame->width) + x,
                        w * 2
                    );
                }
                
                ESP_LOGI(TAG_APP_MAIN, "Waiting for WIFI...");
                xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
                
                xEventGroupClearBits(s_app_event_group, FRAME_ACK_BIT);
                
                // NEW: Updated log message to include all requested info
                ESP_LOGI(TAG_APP_MAIN, "Starting transfer for frame  %d, size: %zu Bytes, Box: [x=%d, y=%d, w=%d, h=%d]",
                         (int)frame_id, cropped_len, x, y, w, h);
                
                char start_msg[128];
                snprintf(start_msg, sizeof(start_msg), "{\"type\":\"frame_start\", \"size\":%zu, \"id\":%d}", cropped_len, (int)frame_id);
                if(websocket_send_text(start_msg) != ESP_OK) { break; }
                vTaskDelay(pdMS_TO_TICKS(10));

                uint8_t *p_buffer = cropped_buf;
                size_t remaining = cropped_len;
                while (remaining > 0) {
                    size_t to_send = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
                    if (websocket_send_frame(p_buffer, to_send) != ESP_OK) {
                        ESP_LOGE(TAG_APP_MAIN, "Failed to send a chunk for frame %d", (int)frame_id);
                        remaining = 1; 
                        break; 
                    }
                    p_buffer += to_send;
                    remaining -= to_send;
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                if (remaining > 0) { break; }

                ESP_LOGI(TAG_APP_MAIN, "Finished sending chunks for frame %d", (int)frame_id);
                if(websocket_send_text("{\"type\":\"frame_end\"}") != ESP_OK) { break; }

                EventBits_t bits = xEventGroupWaitBits(s_app_event_group, FRAME_ACK_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(SERVER_ACK_TIMEOUT_MS*1000));
                if (bits & FRAME_ACK_BIT) {
                    ESP_LOGI(TAG_APP_MAIN, "Got ACK for frame %d!", (int)frame_id);
                } else {
                    ESP_LOGE(TAG_APP_MAIN, "No ACK for frame %d within %d ms.", (int)frame_id, SERVER_ACK_TIMEOUT_MS*1000);
                }

            } while(0);

            esp_camera_fb_return(full_frame);
            ESP_LOGI(TAG_APP_MAIN, "Frame buffer released.");
            if (cropped_buf) {
                free(cropped_buf);
                ESP_LOGI(TAG_APP_MAIN, "Cropped frame buffer released.");
            }
            free(face_data);
            
            ESP_LOGI(TAG_APP_MAIN, "Halt camera for %d sec.", POST_DETECTION_COOLDOWN_S);
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
    xQueueFaceFrame = xQueueCreate(FRAME_QUEUE_SIZE, sizeof(face_to_send_t *)); 
    
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);
    register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueFaceFrame);
    
    xTaskCreate(face_sending_task, "face_sender_task", 4096, NULL, 5, NULL);
#if HEARTBEAT_ON
    ESP_LOGI(TAG_APP_MAIN, "Ping ON, every %d sec.", HEARTBEAT_INTERVAL_S);
    xTaskCreate(heartbeat_task, "heartbeat_task", 3072, NULL, 5, NULL);
#endif
    ESP_LOGI(TAG_APP_MAIN, "App started all workers.");
}