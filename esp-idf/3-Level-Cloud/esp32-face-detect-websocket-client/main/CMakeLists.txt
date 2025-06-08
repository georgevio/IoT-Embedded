idf_component_register(SRCS "app_main.cpp" "wifi.c" "websocket_client.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES 
                                esp_websocket_client  # esp_websocket_client.h
                                esp_psram        # PSRAM functionalities
                                esp32-camera     # esp_camera.h

                       PRIV_REQUIRES modules # who_camera.h, who_human_face_detection.hpp
                      )