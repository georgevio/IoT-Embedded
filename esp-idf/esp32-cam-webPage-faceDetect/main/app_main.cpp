#include "who_camera.h"
#include "who_human_face_detection.hpp"
// #include "app_wifi.h" // NOT used anymore, using local Wi-Fi functions
#include "wifi.h"       // local WiFi header
#include "app_httpd.hpp"
#include "app_mdns.h"
#include "nvs_flash.h"  // For NVS
#include "esp_log.h"    // For ESP_LOGI

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;
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

    //app_wifi_main(); // NOT used anymore, using local Wi-Fi functions

    // Call local WiFi initialization function
    ESP_LOGI(TAG_APP_MAIN, "Initializing WiFi using local wifi.c...");
    wifi_init_sta(); // SET credentials to secret.h (already in .gitignore)

    ESP_LOGI(TAG_APP_MAIN, "Creating Queues...");
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t*));
    xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t*));
    if (xQueueAIFrame == NULL || xQueueHttpFrame == NULL) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create queues.");
        return;
    }

    ESP_LOGI(TAG_APP_MAIN, "Registering Camera...");
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);

    ESP_LOGI(TAG_APP_MAIN, "Initializing mDNS...");
    app_mdns_main();

    ESP_LOGI(TAG_APP_MAIN, "Registering Human Face Detection...");
    register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueHttpFrame);

    ESP_LOGI(TAG_APP_MAIN, "Registering HTTPD Server...");
    register_httpd(xQueueHttpFrame, NULL, true); // 'true' means start web server?

    ESP_LOGI(TAG_APP_MAIN, "app_main setup finished.");
}