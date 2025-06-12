#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_camera.h"

// George struct for the bounding box.
typedef struct {
    int x;
    int y;
    int w;
    int h;
} face_box_t;


typedef struct {
    camera_fb_t* fb;
    face_box_t box;
    uint32_t id; // The struct with a unique ID.
} face_to_send_t;


void register_human_face_detection(const QueueHandle_t frame_i,
    const QueueHandle_t event,
    const QueueHandle_t result,
    const QueueHandle_t frame_o);