/**
 * @file websocket_server.h
 * @brief WebSocket server header
 *
 * WebSocket server init, client connection(s), and message sending (ping/pong).
 */

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

 /**
  * @brief Starts the WebSocket server.
  *
  * Initializes the HTTP server and registers WebSocket URI handlers.
  *
  * @return esp_err_t ESP_OK on success, or an error code if the server cannot start.
  */
esp_err_t start_websocket_server(void);

/**
 * @brief Stops the WebSocket server.
 *
 * @return esp_err_t ESP_OK on success, or an error code.
 */
esp_err_t stop_websocket_server(void);

/**
 * @brief Sends an asynchronous text message to all connected WebSocket clients.
 *
 * @param data The null-terminated string data to send.
 * @return esp_err_t ESP_OK if the message was successfully queued for sending to at least one client
 * ESP_ERR_NOT_FOUND if no clients are connected
 * or other error codes on failure.
 */
esp_err_t websocket_server_send_text_all(const char* data);

/**
 * @brief Sends an asynchronous text message to a specific WebSocket client.
 *
 * @param fd The file of the client's socket.
 * @param data The null-terminated string data to send.
 * @return esp_err_t ESP_OK if the message was successfully queued, or an error code.
 */
esp_err_t websocket_server_send_text_client(int fd, const char* data);

/**
 * @brief Checks if any WebSocket clients are currently connected.
 *
 * @return true if at least one client is connected, false otherwise. Use for further functionalities.
 */
bool websocket_server_is_client_connected(void);

#endif // WEBSOCKET_SERVER_H
