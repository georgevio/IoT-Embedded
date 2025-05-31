 #include "esp_camera.h"
 #include "esp_log.h"
  #include "config.h"

static const char *TAG = "Camera";

bool camera_init() {
    if (!ENABLE_CAMERA) {
        ESP_LOGW(TAG, "Camera initialization disabled in config.");
        return false;
    }

    esp_err_t err = esp_camera_init(&CAMERA_CONFIG);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return false;
    }

    ESP_LOGI(TAG, "Camera initialized successfully!");
    return true;
}
