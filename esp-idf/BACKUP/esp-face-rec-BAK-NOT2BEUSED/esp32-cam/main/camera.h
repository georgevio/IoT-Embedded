#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "esp_log.h"

/**
 * @brief Initializes the ESP32-CAM module.
 * 
 * @return true if the initialization is successful, false otherwise.
 */
bool camera_init();

#endif // CAMERA_H
