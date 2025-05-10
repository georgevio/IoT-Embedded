idf_component_register(
    SRCS "main.c"
         "camera.c"
         "wifi.c"
         "http.c"
    INCLUDE_DIRS "."
    REQUIRES esp32-camera esp_http_server
    PRIV_REQUIRES esp_wifi nvs_flash lwip esp_psram esp_timer
)
