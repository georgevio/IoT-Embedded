idf_component_register(SRCS "main.c" "mqtt.c" "bme280.c" "wifi.c"
                      INCLUDE_DIRS "."
                      EMBED_TXTFILES "../certificates/AmazonRootCA1.pem"
                                     "../certificates/new_certificate.pem"
                                     "../certificates/new_private.key"
)