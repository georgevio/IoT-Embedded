idf_component_register(SRCS "app_main.cpp" "wifi.c" "websocket_client.c"
                       INCLUDE_DIRS "."
                       REQUIRES 
                                esp_websocket_client  # Provides esp_websocket_client.h
                                esp_psram        # For PSRAM functionalities
                                esp32-camera     # Provides esp_camera.h

                       PRIV_REQUIRES # For private headers/dependencies used only by this component's .c/.cpp files
                                modules # Provides who_camera.h and who_human_face_detection.hpp
                      )