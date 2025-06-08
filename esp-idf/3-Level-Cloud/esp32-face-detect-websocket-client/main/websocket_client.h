#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

// imported defintions from ESP-IDF, "as-is".

#include "esp_err.h"
#include "esp_camera.h"

struct EventGroupDef_t;
typedef struct EventGroupDef_t* EventGroupHandle_t;

#ifdef __cplusplus
extern "C" {
#endif

void websocket_client_start(EventGroupHandle_t event_group);
void websocket_client_stop(void);
esp_err_t websocket_send_frame(camera_fb_t *fb);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_CLIENT_H