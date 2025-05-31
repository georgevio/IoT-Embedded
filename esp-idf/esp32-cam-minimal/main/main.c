#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO 4  // ESP32-CAM onboard LED pin

void app_main(void) {
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_level(LED_GPIO, 1); // Turn LED ON
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait 500ms
    gpio_set_level(LED_GPIO, 0); // Turn LED OFF
    //vTaskDelay(pdMS_TO_TICKS(500)); // Wait 500ms

}