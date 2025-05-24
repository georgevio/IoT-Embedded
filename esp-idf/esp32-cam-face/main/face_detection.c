#include "face_detection.h"
#include "esp_log.h"
//#include "esp_who.h" // ..\components> git clone https://github.com/espressif/esp-who.git
#include <stdio.h>
#include <string.h> // For memset
#include "debug.h"  // central debug management

static const char *TAG_FACE = "FACE_DETECT";
static bool initialized = false;

esp_err_t face_detection_init() {
    DEBUG_PRINT(TAG_FACE, "Initializing face detection module..."); // Using DEBUG_PRINT
    // Initialize ESP-WHO face detection here (replace with actual ESP-WHO init)
    // esp_err_t err = esp_who_face_detection_init();
    // if (err != ESP_OK) {
    //     ERROR_PRINT(TAG_FACE, "Face detection initialization failed: %d", err); // Using ERROR_PRINT
    //     return err;
    // }
    initialized = true;
    DEBUG_PRINT(TAG_FACE, "Face detection module initialized."); // Using DEBUG_PRINT
    return ESP_OK;
}

esp_err_t detect_faces(camera_fb_t *frame_buffer, face_detection_results_t *results) {
    if (!initialized) {
        ERROR_PRINT(TAG_FACE, "Face detection module not initialized!"); 
        return ESP_FAIL;
    }

    if (!frame_buffer || !results) {
        ERROR_PRINT(TAG_FACE, "Invalid input parameters.");
        return ESP_ERR_INVALID_ARG;
    }

    memset(results, 0, sizeof(face_detection_results_t)); // Clear previous results

    //    Example (PLACEHOLDER):
    //    esp_who_face_detect(frame_buffer->buf, frame_buffer->width, frame_buffer->height, PIXEL_FORMAT, results);
    //    results->num_faces = ...; // Update the number of detected faces
    //    // Populate the results->faces array with bounding box coordinates

    return ESP_OK;
}

void face_detection_deinit() {
    DEBUG_PRINT(TAG_FACE, "Deinitializing face detection module..."); // Using DEBUG_PRINT

    // esp_who_face_detection_deinit();
    initialized = false;
    DEBUG_PRINT(TAG_FACE, "Face detection module deinitialized."); // Using DEBUG_PRINT
}