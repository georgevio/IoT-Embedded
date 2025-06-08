#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include "esp_err.h"
#include "esp_camera.h"   // For camera_fb_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes and starts the WebSocket client.
 * Connects to the URI defined in WEBSOCKET_URI (secret.h).
 */
void websocket_client_start(void);

/**
 * @brief Stops and destroys the WebSocket client.
 */
void websocket_client_stop(void);

/**
 * @brief Sends camera frame buffer data over WebSocket.
 * @param fb Pointer to the camera frame buffer.
 * @return esp_err_t ESP_OK on success, or an error code otherwise.
 */
esp_err_t websocket_send_frame(camera_fb_t *fb);

/**
 * @brief Checks if the WebSocket client is connected.
 * @return true if connected, false otherwise.
 */
bool is_websocket_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_CLIENT_H