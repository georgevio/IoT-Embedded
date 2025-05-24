#ifndef FACE_DETECTION_H_
#define FACE_DETECTION_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_camera.h"

// Structure to hold face detection results (you might need to adapt this
// based on the ESP-WHO library's output)
typedef struct {
    uint8_t num_faces;
    // Example: Array to store bounding box coordinates (x, y, width, height)
    uint16_t faces[10][4]; // Assuming a maximum of 10 faces
} face_detection_results_t;

esp_err_t face_detection_init();
esp_err_t detect_faces(camera_fb_t *frame_buffer, face_detection_results_t *results);

void face_detection_deinit();

#endif // FACE_DETECTION_H_