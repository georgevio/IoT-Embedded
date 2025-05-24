#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "esp_camera.h"

//esp_err_t 
void camera_init();
esp_err_t camera_capture(camera_fb_t **frame_buffer);
void camera_fb_return(camera_fb_t *frame_buffer);

#endif // CAMERA_H