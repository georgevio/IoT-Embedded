set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components)

# Include certificates as binary files
set(CERT_FILES
    ${CMAKE_CURRENT_LIST_DIR}/certificates/AmazonRootCA1.pem
    ${CMAKE_CURRENT_LIST_DIR}/certificates/new_certificate.pem
    ${CMAKE_CURRENT_LIST_DIR}/certificates/new_private.key
)

# Register the component (main app)
idf_component_register(SRCS "main.c" "wifi.c" "mqtt.c"
                      INCLUDE_DIRS "." "../certificates"
					  PRIV_REQUIRES driver
                      EMBED_TXTFILES "../certificates/AmazonRootCA1.pem"
                                    "../certificates/new_certificate.pem"
                                    "../certificates/new_private.key"
                      REQUIRES mqtt esp_wifi nvs_flash)