#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "wifi.h"
#include "camera.h"
#include "http.h"
#include "esp_event.h"
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
#include "debug.h"  // Include centralized debug management

/*************** SETTINGS FOR RUNTIME DEBUGGING ************/
#define WI_FI_ON 1 // turn off for debugging runtime errors
#define WEB_SERVER_ON 1 // turn off for debugging runtime errors
#define CONFIG_ESP_PSRAM_ENABLE 1 // test this depending on the app
#define CHANGE_LOG_LEVEL_ON_RUNTIME 0 // there is a keyboard read in debug.c
/**********************************************************/

static const char *TAG_MAIN = "MAIN";

/**
 * @brief Application entry point.
 */
void app_main(void) {
    DEBUG_PRINT(TAG_MAIN, "Starting ESP32-CAM application...");

    // Print heap info **only if log level is DEBUG**
    if (global_log_level == LOG_DEBUG) {
        print_heap_info(TAG_MAIN);
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        DEBUG_PRINT(TAG_MAIN, "Erasing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    DEBUG_PRINT(TAG_MAIN, "NVS initialized.");

    // Print heap info **only if log level is DEBUG**
    if (global_log_level == LOG_DEBUG) {
        print_heap_info(TAG_MAIN);
    }

    // Camera initialization
    camera_init();

#if WI_FI_ON
    DEBUG_PRINT(TAG_MAIN, "WI_FI_ON==1, Initializing WiFi...");
    wifi_init_sta();
#ifdef WEB_SERVER_ON
    DEBUG_PRINT(TAG_MAIN, "WEB_SERVER_ON==1, starting webserver...");
    start_webserver();
#else
    DEBUG_PRINT(TAG_MAIN, "WEB_SERVER_ON==0, webserver disabled...");
#endif
#else
    DEBUG_PRINT(TAG_MAIN, "WI_FI_ON==0, wifi not started.");
#endif

    // Print heap info **only if log level is DEBUG**
    if (global_log_level == LOG_DEBUG) {
        print_heap_info(TAG_MAIN);
    }

#if CHANGE_LOG_LEVEL_ON_RUNTIME
    // Periodically check for runtime debug mode changes. Check for blocking event
    while (1) {
        check_serial_input();
        vTaskDelay(pdMS_TO_TICKS(2000));  // Delay for input handling
    }
#endif
    
    DEBUG_PRINT(TAG_MAIN, "app_main() reached bottom...");
}
