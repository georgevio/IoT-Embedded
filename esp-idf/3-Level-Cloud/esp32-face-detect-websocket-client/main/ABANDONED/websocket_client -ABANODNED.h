#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "who_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

void websocket_client_start(EventGroupHandle_t event_group);
void websocket_client_stop(void);
esp_err_t websocket_send_heartbeat(void);
esp_err_t websocket_send_frame(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_CLIENT_H