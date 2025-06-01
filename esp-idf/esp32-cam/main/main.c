#include "esp_camera.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "lwip/sockets.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "secret.h" // Contains WIFI_SSID and WIFI_PASSWORD

// Camera Pin Definitions (AI-Thinker ESP32-CAM)
/* NOTE: If the camera fails to initialize, it is because of the following 
 * settings.
 * Check with the arduino code first if the camera is working ok, and then
 * transfer the settings here. 
 */
#define PWDN_GPIO_NUM     32 //change this if you want to see the camera errors
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

static const char *TAG = "ESP32-CAM";

/*************** SETTINGS FOR MINIMAL TESTING *******************/
#define WI_FI_ON 1 // you may disable wifi con, if testing camera core functions
#define CAPTURE_IMAGE 1 // Testing camera capture. Can be disabled
/***************************************************************/

#if WI_FI_ON // wifi initialization

// Initialize WiFi
void wifi_init_sta() {
    ESP_LOGI(TAG, "Initializing WiFi...");
    if (strcmp(WIFI_SSID, "Your Wi-Fi SSID") == 0) { // WIFI MUST be set in secret.h
        ESP_LOGE(TAG, "WIFI is on, but SSID not set!");
        return;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID, // get it from secret.h
            .password = WIFI_PASSWORD, // get it from secret.h
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for WiFi connection... (Note: No connection event handling in this basic example)");
    vTaskDelay(pdMS_TO_TICKS(5000));
}
#endif // wifi initialization

// Initialize Camera
esp_err_t camera_init() {
    if (PWDN_GPIO_NUM != -1) {
        ESP_LOGI(TAG, "Toggling PWDN pin (%d)", PWDN_GPIO_NUM);
        gpio_config_t conf = {0};
        conf.pin_bit_mask = 1LL << PWDN_GPIO_NUM;
        conf.mode = GPIO_MODE_OUTPUT;
        gpio_config(&conf);

        gpio_set_level((gpio_num_t)PWDN_GPIO_NUM, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level((gpio_num_t)PWDN_GPIO_NUM, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

/* this is transfered from Arduino code. It causes many errors/warnings */
#if RESET_GPIO_NUM != -1
    ESP_LOGI(TAG, "Toggling RESET pin (%d)", RESET_GPIO_NUM);
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
    ESP_LOGI(TAG, "PSRAM available: %s, Size: %zu bytes", psramAvailable ? "Yes" : "No", psram_size);

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
        .frame_size = FRAMESIZE_UXGA,
        .jpeg_quality = 12,
        .fb_count = psramAvailable ? 2 : 1,
        .fb_location = psramAvailable ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM,
        .grab_mode = CAMERA_GRAB_LATEST
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed: 0x%x (%s)", err, esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Camera component initialized.");

    return ESP_OK;
}

// Capture and Log Image Details
esp_err_t camera_capture() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera Capture Failed!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Captured Image: %d x %d, Format: %d, Size: %zu bytes",
             fb->width, fb->height, fb->format, fb->len);

    esp_camera_fb_return(fb);
    return ESP_OK;
}

// Main 
void app_main() {
    ESP_LOGI(TAG, "Starting ESP32-CAM application...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was corrupt or version mismatch, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Initialized.");

#if WI_FI_ON 
    wifi_init_sta();
#endif

    esp_err_t cam_err = camera_init();
    if (cam_err == ESP_OK) {
        ESP_LOGI(TAG, "Camera Initialized Successfully!");

#if CAMERA_TEST
        // If you want to test capture immediately after init:
        vTaskDelay(pdMS_TO_TICKS(1000));
        camera_capture();
#endif
    } else {
        ESP_LOGE(TAG, "Camera Initialization Failed! Error: 0x%x (%s)", cam_err, esp_err_to_name(cam_err));
    }

    ESP_LOGI(TAG, "Application main finished setup. Device is running.");

    uint8_t mac_addr[6];
    char mac_str[18];
    esp_err_t mac_ret = esp_base_mac_addr_get(mac_addr);
    if (mac_ret == ESP_OK) {
        sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac_addr[0], mac_addr[1], mac_addr[2],
                mac_addr[3], mac_addr[4], mac_addr[5]);
        ESP_LOGI(TAG, "ESP32-CAM Base MAC: %s", mac_str);
    } else {
        ESP_LOGE(TAG, "Failed to get base MAC address");
    }
}