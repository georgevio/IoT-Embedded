idf_component_register(SRCS "main.c" "mqtt.c" "wifi.c" "websocket_server.c"
                      INCLUDE_DIRS "."
                      EMBED_TXTFILES "../certificates/AmazonRootCA1.pem"
                                     "../certificates/new_certificate.pem"
                                     "../certificates/new_private.key"
					REQUIRES esp_http_server esp_netif nvs_flash
					PRIV_REQUIRES esp_wifi mqtt
)