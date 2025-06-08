#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi.h"
#include "websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"

static EventGroupHandle_t s_app_event_group;
// new logic for handling WiFi and WebSocket events, adapted from ESP-IDF examples
const static int WIFI_CONNECTED_BIT = (1 << 0);
const static int WEBSOCKET_CONNECTED_BIT = (1 << 1);
const static int FRAME_QUEUE_SIZE = 5;

//basic hanlders fro the camera feed and face detection
static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueFaceFrame = NULL;
static const char* TAG_APP_MAIN = "MAIN_APP";

// the face has already been detected and is about to be sent
static void face_sending_task(void* pvParameters) {
    camera_fb_t* frame = NULL;
    while (true) {
        if (xQueueReceive(xQueueFaceFrame, &frame, portMAX_DELAY)) {
            if (!frame) continue;

            ESP_LOGI(TAG_APP_MAIN, "Frame ready. Waiting for network...");
            xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
            
            ESP_LOGI(TAG_APP_MAIN, "Network ready. Sending frame.");
            if (websocket_send_frame(frame) != ESP_OK) {
                ESP_LOGE(TAG_APP_MAIN, "Failed to send frame.");
            }
            esp_camera_fb_return(frame);
        }
    }
}

//event handler for WiFi and IP events. without this, frames will halt being sent
static void app_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG_APP_MAIN, "WiFi disconnected.");
        xEventGroupClearBits(s_app_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT);
        websocket_client_stop();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG_APP_MAIN, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_app_event_group, WIFI_CONNECTED_BIT);
    }
}

extern "C" void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    s_app_event_group = xEventGroupCreate();
    
    wifi_init_sta();
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &app_event_handler, NULL));

    xQueueAIFrame = xQueueCreate(FRAME_QUEUE_SIZE, sizeof(camera_fb_t*));
    xQueueFaceFrame = xQueueCreate(FRAME_QUEUE_SIZE, sizeof(camera_fb_t*));
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);
    register_human_face_detection(xQueueAIFrame, xQueueFaceFrame, NULL, NULL, true);
    
    xTaskCreate(face_sending_task, "face_sender_task", 4096, NULL, 5, NULL);

    while (true) {
        ESP_LOGI(TAG_APP_MAIN, "Waiting for WiFi connection...");
        xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        
        ESP_LOGI(TAG_APP_MAIN, "WiFi connected. Starting WebSocket client.");
        websocket_client_start(s_app_event_group);
        
        ESP_LOGI(TAG_APP_MAIN, "System running. Waiting for network disconnection...");
        xEventGroupWaitBits(s_app_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdTRUE, portMAX_DELAY); // Wait for bit to be cleared
        
        ESP_LOGW(TAG_APP_MAIN, "WiFi connection lost. Waiting for reconnect...");
    }
}