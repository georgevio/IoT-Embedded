/* Optimized code to fit into ESP32-S3-WROOM-1 */

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "secrets.h"  // Keeps your credentials secure
#include "mqtt_config.h"  // Includes necessary MQTT configurations

static esp_mqtt_client_handle_t client;

// WiFi Event Handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    }
}

// Initialize WiFi using ESP-IDF APIs
void initWiFi() {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// MQTT Event Handler (Fix: Explicit Casting)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data; // Explicit cast

    if (event->event_id == MQTT_EVENT_CONNECTED) {
        esp_mqtt_client_publish(client, MQTT_TOPIC, "36", 0, 1, 0); // Send initial data
    }
}

void initMQTT() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    
    mqtt_cfg.broker.address.uri = MQTT_SERVER; // Explicit assignment
    mqtt_cfg.broker.address.port = MQTT_PORT; // Explicit assignment

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, &mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void setup() {
    initWiFi();
    initMQTT();
}

void loop() {
    static unsigned long lastPublishTime = 0;
    if (millis() - lastPublishTime > 90000) {
        esp_mqtt_client_publish(client, MQTT_TOPIC, "36", 0, 1, 0); // Example sensor value
        lastPublishTime = millis();
    }
}
