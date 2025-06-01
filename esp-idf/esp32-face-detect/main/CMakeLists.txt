idf_component_register(SRCS "app_main.cpp" "wifi.c"
                      INCLUDE_DIRS "."
					REQUIRES esp_netif nvs_flash 
					PRIV_REQUIRES esp_wifi esp32-camera modules
)