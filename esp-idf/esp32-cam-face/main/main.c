#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "debug.h"
#include "system_init.h"

static const char *TAG_MAIN = "MAIN";

void app_main(void) {
    esp_log_level_set(TAG_MAIN, ESP_LOG_INFO);

    DEBUG_PRINT(TAG_MAIN, "Calling system initialization system_init().");
    system_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        print_heap_info(TAG_MAIN);
    }
}

// Task implementations are in camera.c and face_detection.c