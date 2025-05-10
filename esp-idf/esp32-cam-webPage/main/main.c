#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "wifi.h"
#include "camera.h"
#include "http.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_timer.h"

/*************** SETTINGS FOR DEBUGGING *******************/
#define WI_FI_ON 1 // Enable WiFi. Set to 0 to disable WiFi for testing camera only.
#define WEB_SERVER_ON 1 // Debugging ONLY, keep it on
#define CONFIG_ESP_PSRAM_ENABLE 1 // Enable PSRAM
/**********************************************************/

static const char *TAG_MAIN = "MAIN";

/**
 * @brief Print heap information.
 */
static void print_heap_info(const char *tag) {
    ESP_LOGI(tag, "Free internal heap: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#ifdef CONFIG_ESP_PSRAM_ENABLE
    ESP_LOGI(tag, "Free PSRAM heap: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif
}

/**
 * @brief Application entry point.
 */
void app_main(void) {
    ESP_LOGI(TAG_MAIN, "Starting ESP32-CAM application...");
    print_heap_info(TAG_MAIN);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG_MAIN, "Erasing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG_MAIN, "NVS initialized.");
    print_heap_info(TAG_MAIN);
    
    // Camera initialization - No extra logging here, check camera.c
    camera_init();

#if WI_FI_ON
    ESP_LOGI(TAG_MAIN, "WI_FI_ON==1, Initializing WiFi...");
    wifi_init_sta();
#ifdef WEB_SERVER_ON
    ESP_LOGI(TAG_MAIN, "WEB_SERVER_ON==1, starting webserver...");
    start_webserver();
#else
    ESP_LOGW(TAG_MAIN, "WEB_SERVER_ON==0, webserver disabled...");
#endif
#else
    ESP_LOGW(TAG_MAIN, "WI_FI_ON==0, wifi not started.");
#endif

    print_heap_info(TAG_MAIN);
}
