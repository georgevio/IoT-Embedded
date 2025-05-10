#include "camera.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_psram.h" 
#include "esp_camera.h"
static const char *TAG_CAMERA = "CAMERA";

// Camera Pin Definitions (AI-Thinker ESP32-CAM)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_GPIO 4  // ESP32-CAM onboard LED

//esp_err_t // is there a need to return the error?
void camera_init() {
    if (PWDN_GPIO_NUM != -1) {
        ESP_LOGI(TAG_CAMERA, "Toggling PWDN pin (%d)", PWDN_GPIO_NUM);
        gpio_config_t conf = {0};
        conf.pin_bit_mask = 1LL << PWDN_GPIO_NUM;
        conf.mode = GPIO_MODE_OUTPUT;
        gpio_config(&conf);
        gpio_set_level((gpio_num_t)PWDN_GPIO_NUM, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level((gpio_num_t)PWDN_GPIO_NUM, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

#if RESET_GPIO_NUM != -1
    ESP_LOGI(TAG_CAMERA, "Toggling RESET pin (%d)", RESET_GPIO_NUM);
    gpio_config_t conf = {0};
    conf.pin_bit_mask = 1LL << RESET_GPIO_NUM;
    conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&conf);
    gpio_set_level((gpio_num_t)RESET_GPIO_NUM, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)RESET_GPIO_NUM, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
#endif

    size_t psram_size = esp_psram_get_size();
    bool psramAvailable = (psram_size > 0);
    ESP_LOGI(TAG_CAMERA, "PSRAM available: %s, Size: %zu bytes", psramAvailable ? "Yes" : "No", psram_size);

    camera_config_t camera_config = {
        .pin_pwdn = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,
        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sccb_sda = SIOD_GPIO_NUM,
        .pin_sccb_scl = SIOC_GPIO_NUM,
        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 15,
        .fb_count = 1,
        .fb_location = psramAvailable ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM,
        //.fb_count = psramAvailable ? 2 : 1,
        //.fb_location = psramAvailable ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM,
        .grab_mode = CAMERA_GRAB_LATEST
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_CAMERA, "Camera Init Failed: 0x%x (%s)", err, esp_err_to_name(err));
        //return err;
    }
    ESP_LOGI(TAG_CAMERA, "Camera component initialized.");
    // Configure LED GPIO as output
    esp_rom_gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    // Turn on LED momentarily
    gpio_set_level(LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second
    gpio_set_level(LED_GPIO, 0);
    
    //return ESP_OK;
}

esp_err_t camera_capture(camera_fb_t **frame_buffer) {
    *frame_buffer = esp_camera_fb_get();
    if (!*frame_buffer) {
        ESP_LOGE(TAG_CAMERA, "Camera capture failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void camera_fb_return(camera_fb_t *frame_buffer) {
    esp_camera_fb_return(frame_buffer);
}