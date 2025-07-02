#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the image processor module.
 */
esp_err_t image_processor_init(void);

/**
 * @brief Handles a new complete image received from any source.
 *
 * @param image_buffer Pointer to the raw image data (RGB888).
 * @param image_len The total size of the image buffer in bytes.
 * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t image_processor_handle_new_image(uint8_t *image_buffer, size_t image_len, int width, int height);

#ifdef __cplusplus
}
#endif