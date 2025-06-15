#include "esp_log.h"
#include "nvs_flash.h"
#include "image_processor.hpp"

static const char *TAG = "APP_MAIN";

// face recong based on esp-who. no functionailities....
extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP-WHO face recognition start...");

    ImageProcessor *app_processor = new ImageProcessor();
    app_processor->run_recognition_test();

    ESP_LOGI(TAG, "App finished...");

    delete app_processor;
}
