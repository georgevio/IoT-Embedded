#include "esp_log.h"
#include "image_processor.h"
#include "face_recognizer.hpp" // Directly include the C++ header

static const char* TAG = "IMAGE_PROCESSOR";

// Create a static instance of the FaceRecognizer.
// Its constructor is called automatically at startup.
static FaceRecognizer g_recognizer;

// This function is exposed to C code via image_processor.h
esp_err_t image_processor_init(void) {
    ESP_LOGI(TAG, "Image processor initialized (C++).");
    // The g_recognizer object is already constructed by this point.
    return ESP_OK;
}

// This function is also exposed to C code via image_processor.h
esp_err_t image_processor_handle_new_image(uint8_t *image_buffer, size_t image_len, int width, int height) {
    ESP_LOGI(TAG, "New image received (%d bytes, %dx%d).", (int)image_len, width, height);

    // Directly call the C++ method on the static object
    int face_id = g_recognizer.recognize_face(image_buffer, width, height);

    if (face_id >= 0) {
        ESP_LOGI(TAG, "********************************");
        ESP_LOGI(TAG, "* RESULT: FACE RECOGNIZED! ID: %d *", face_id);
        ESP_LOGI(TAG, "********************************");
    } else {
        ESP_LOGW(TAG, "********************************");
        ESP_LOGW(TAG, "* RESULT: UNKNOWN FACE / ERROR *");
        ESP_LOGW(TAG, "********************************");
    }

    return ESP_OK;
}