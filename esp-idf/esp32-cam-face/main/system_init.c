#include "system_init.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "camera.h"
#include "wifi.h"
#include "http.h"
#include "debug.h" // Include debug header to access defines

#if ENABLE_FACE_DETECT
#include "face_detection.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/queue.h"    
QueueHandle_t frame_queue; // Needs to be declared in a common scope
#endif //ENABLE_FACE_DETECT

static const char *TAG_SYS_INIT = "SYS_INIT";

void system_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        DEBUG_PRINT(TAG_SYS_INIT, "Erasing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    DEBUG_PRINT(TAG_SYS_INIT, "NVS initialized.");

    camera_init();

#if WI_FI_ON
    DEBUG_PRINT(TAG_SYS_INIT, "WI_FI_ON==1, connecting to WiFi...");
    wifi_init_sta();
#else
    DEBUG_PRINT(TAG_SYS_INIT, "WI_FI_ON==0, wifi not started.");
#endif //#if WI_FI_ON

#if WEB_SERVER_ON
    DEBUG_PRINT(TAG_SYS_INIT, "WEB_SERVER_ON==1, call start_webserver()...");
    start_webserver();
#else
    DEBUG_PRINT(TAG_SYS_INIT, "WEB_SERVER_ON==0, webserver disabled...");
#endif //#if WEB_SERVER_ON

#if ENABLE_FACE_DETECT
    DEBUG_PRINT(TAG_SYS_INIT, "ENABLE_FACE_DETECT==1, calling face_detection_init()");

    esp_err_t face_init_result = face_detection_init();
    if (face_init_result != ESP_OK) {
        ERROR_PRINT(TAG_SYS_INIT, "Face detection init error: %d", face_init_result);
    }
    

    // Create the frame queue
    frame_queue = xQueueCreate(10, sizeof(camera_fb_t *));
    if (!frame_queue) {
        ERROR_PRINT(TAG_SYS_INIT, "Failed to create frame queue!");
        return;
    }
#else
    DEBUG_PRINT(TAG_SYS_INIT, "ENABLE_FACE_DETECT==0");
#endif //#if ENABLE_FACE_DETECT
}