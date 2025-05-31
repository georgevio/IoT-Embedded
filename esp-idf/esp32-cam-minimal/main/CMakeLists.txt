idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
	REQUIRES esp32-camera
    PRIV_REQUIRES nvs_flash lwip esp_psram
)
