// system_init.h
#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

void system_init();

#if ENABLE_FACE_RECOGNITION
extern QueueHandle_t frame_queue; 
#endif

#endif // SYSTEM_INIT_H