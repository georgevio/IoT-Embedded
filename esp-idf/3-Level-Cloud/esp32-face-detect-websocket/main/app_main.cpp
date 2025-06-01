#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi.h"
#include "websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_netif.h"

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueFaceFrame = NULL;
static const char* TAG_APP_MAIN = "MAIN_APP";

static volatile bool wifi_has_ip = false;
static SemaphoreHandle_t wifi_connected_semaphore = NULL; // Can be used for synchronization

// send frames with detected faces via WebSocket
static void face_sending_task(void* pvParameters)
{
    camera_fb_t* frame = NULL;

    while (true)
    {
        if (xQueueReceive(xQueueFaceFrame, &frame, portMAX_DELAY))
        {
            if (!frame) {
                ESP_LOGE(TAG_APP_MAIN, "Received NULL frame from xQueueFaceFrame");
                continue;
            }

            // Send frame ONLY if WebSocket is connected and Wi-Fi has an IP
            if (wifi_has_ip && is_websocket_connected())
            {
                ESP_LOGI(TAG_APP_MAIN, "Face detected, WebSocket connected, attempt to send frame.");
                esp_err_t send_err = websocket_send_frame(frame);
                if (send_err != ESP_OK)
                {
                    ESP_LOGE(TAG_APP_MAIN, "Failed to send frame via WebSocket.");
                }
            }
            else
            {
                if (!wifi_has_ip) {
                    ESP_LOGW(TAG_APP_MAIN, "Face detected, WiFi not connected.");
                }
                else if (!is_websocket_connected()) {
                    ESP_LOGW(TAG_APP_MAIN, "Face detected, WebSocket not connected.");
                }
            }
            esp_camera_fb_return(frame);
        }
    }
}

// WiFi events
static void wifi_event_handler_app(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG_APP_MAIN, "WiFi disconnected. Clearing wifi_has_ip flag.");
        wifi_has_ip = false;
        if (is_websocket_connected()) {
            ESP_LOGI(TAG_APP_MAIN, "Stopping WebSocket client due to WiFi disconnection.");
            websocket_client_stop();
        }
        // Reconnection is handled in wifi.c
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG_APP_MAIN, "APP_HANDLER: Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_has_ip = true;

        if (wifi_connected_semaphore != NULL) {
            ESP_LOGI(TAG_APP_MAIN, "APP_HANDLER: Giving wifi_connected_semaphore.");
            xSemaphoreGive(wifi_connected_semaphore);
        }
    }
}

extern "C" void app_main()
{
    ESP_LOGI(TAG_APP_MAIN, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG_APP_MAIN, "Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG_APP_MAIN, "NVS initialized.");

    wifi_connected_semaphore = xSemaphoreCreateBinary();
    if (wifi_connected_semaphore == NULL) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create WiFi semaphore.");
    }

    // wifi_init_sta() calls esp_netif_init() and esp_event_loop_create_default()
    ESP_LOGI(TAG_APP_MAIN, "Initializing WiFi Station mode...");
    wifi_init_sta();

    ESP_LOGI(TAG_APP_MAIN, "Registering application-specific WiFi event handlers...");
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler_app, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_handler_app, NULL));

    ESP_LOGI(TAG_APP_MAIN, "Waiting for WiFi to get IP address...");
    int wifi_connect_retry_count = 0;
    const int max_wifi_connect_retries = 240; // 2 minutes (240 * 500ms)

    while (!wifi_has_ip && wifi_connect_retry_count < max_wifi_connect_retries) {
        vTaskDelay(pdMS_TO_TICKS(500));
        wifi_connect_retry_count++;
        if (wifi_connect_retry_count % 4 == 0) { // Log every 2 seconds
            ESP_LOGI(TAG_APP_MAIN, "Waiting for IP address... (attempt %d/%d)", wifi_connect_retry_count, max_wifi_connect_retries);
        }
    }

    if (wifi_has_ip) {
        ESP_LOGI(TAG_APP_MAIN, "WiFi HAS IP. Start websocket.");
        websocket_client_start(); // Websocket client is started here
        ESP_LOGI(TAG_APP_MAIN, "CALL TO websocket_client_start() HAS RETURNED.");
    }
    else {
        ESP_LOGE(TAG_APP_MAIN, "TIMEOUT for WiFi IP address.");
    }

    ESP_LOGI(TAG_APP_MAIN, "Creating xQueueAIFrame...");
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t*));
    if (xQueueAIFrame == NULL) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create xQueueAIFrame.");
        return;
    }

    ESP_LOGI(TAG_APP_MAIN, "Creating xQueueFaceFrame...");
    xQueueFaceFrame = xQueueCreate(2, sizeof(camera_fb_t*));
    if (xQueueFaceFrame == NULL) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create xQueueFaceFrame.");
        return;
    }

    ESP_LOGI(TAG_APP_MAIN, "Registering Camera...");
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);

    ESP_LOGI(TAG_APP_MAIN, "Registering Face Detection...");
    // Arg1: Input frames, Arg2: Output frames with faces, Arg3: Result metadata, Arg4: Event queue
    register_human_face_detection(xQueueAIFrame, xQueueFaceFrame, NULL, NULL, true);

    ESP_LOGI(TAG_APP_MAIN, "Creating face sending task...");
    xTaskCreate(face_sending_task, "face_sender_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG_APP_MAIN, "app_main DONE. System running.");
}