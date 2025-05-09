idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
	REQUIRES esp32-camera
    PRIV_REQUIRES esp_wifi nvs_flash lwip esp_psram
)
