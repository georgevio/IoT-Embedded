#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "nvs_flash.h"      // For NVS
#include "esp_event.h"      // For esp_event_loop_create_default (needed by wifi)
#include "esp_log.h"        // For ESP_LOGI
#include "wifi.h"      

static QueueHandle_t xQueueAIFrame = NULL;
static const char* TAG_APP_MAIN = "MAIN"; // For logging

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

     // George: check if they are called AT LEAST once.
    // ESP_ERROR_CHECK(esp_netif_init()); // This is called in wifi_init_sta
    // ESP_ERROR_CHECK(esp_event_loop_create_default()); // This is called in wifi_init_sta

    ESP_LOGI(TAG_APP_MAIN, "Initializing WiFi Station mode...");
    wifi_init_sta(); // Initialize and start WiFi

    ESP_LOGI(TAG_APP_MAIN, "Creating AI Frame Queue...");
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t*));
    if (xQueueAIFrame == NULL) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create AI Frame Queue.");
        return; // Or handle error
    }

    ESP_LOGI(TAG_APP_MAIN, "Registering Camera...");
    // Assuming PIXFORMAT_RGB565 and FRAMESIZE_QVGA are correct for particular setup
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);

    ESP_LOGI(TAG_APP_MAIN, "Registering Human Face Detection...");
    // The last 'true' is assumed to be, start detection immediately.
    register_human_face_detection(xQueueAIFrame, NULL, NULL, NULL, true);

    ESP_LOGI(TAG_APP_MAIN, "app_main finished setup.");
	// camera & face detection run in the background. Debug messages are in the appropriate modules.
}