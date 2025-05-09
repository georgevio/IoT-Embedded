/**
 * @file config.h
 * @brief Configuration for ESP32 application modules
 * 
 * Contains configuration flags to enable/disable various modules
 * and their respective settings.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "../certificates/secrets.h"  // Include sensitive configuration
#include "esp_camera.h"

// Module enable flags
#define WIFI_ENABLED  1
#define MQTT_ENABLED  1
#define ENABLE_CAMERA 1

// WiFi configuration
#define WIFI_MAXIMUM_RETRY 5

//MQTT sending interval GLOBAL. maybe needs to be per value?
#define SAMPLING_INTERVAL_MS 120000  // 2 minutes 

static const camera_config_t CAMERA_CONFIG = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = 0,
    .pin_sscb_sda = 26,
    .pin_sscb_scl = 27,
    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1
};

#endif // CONFIG_H