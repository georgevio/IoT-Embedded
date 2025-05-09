#include <Wire.h>
#include <Adafruit_BME280.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "secrets.h"  // Keeps your credentials secure
#include "mqtt_config.h"  // Includes necessary MQTT configurations

static esp_mqtt_client_handle_t client;
Adafruit_BME280 bme; // BME280 sensor instance

/* Define I2C pins for ESP32-S3-WROOM-1 for BME 280. You can use other GPIOs as well */ 
#define SDA_PIN 18
#define SCL_PIN 17

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

// MQTT Event Handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    /* Succesful connection message to MQTT broker. Can be disabled */
    if (event->event_id == MQTT_EVENT_CONNECTED) {
        esp_mqtt_client_publish(client, MQTT_TOPIC, "ESP32-S3 Connected", 0, 1, 0);
    }
}

void initMQTT() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    
    mqtt_cfg.broker.address.uri = MQTT_SERVER;
    mqtt_cfg.broker.address.port = MQTT_PORT;

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, &mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

// Initialize BME280 Sensor
void initBME280() {
    Wire.begin(SDA_PIN, SCL_PIN); // Set I2C pins as above
    if (!bme.begin(0x76)) {  // Check if the sensor is connected
        Serial.println("\nBME280 sensor NOT found!");
        while (1);
    }
    Serial.println("\nBME280 sensor found!");
}

// Publish Sensor Data to MQTT
void publishSensorData() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F; // Convert to hPa

    char payload[100];
    snprintf(payload, sizeof(payload), "{\"temp\": %.2f, \"humidity\": %.2f, \"pressure\": %.2f}", 
             temperature, humidity, pressure);
    
    // Print to Serial Monitor for Debugging
    Serial.print("Payload: ");
    Serial.println(payload);

    esp_mqtt_client_publish(client, MQTT_TOPIC, payload, 0, 1, 0);
}

void setup() {
    Serial.begin(115200); // Initialize Serial Monitor
    initWiFi();
    initMQTT();
    initBME280();
}

void loop() {
    static unsigned long lastPublishTime = 0;
    if (millis() - lastPublishTime > 30000) {  // Publish every 5 min
        publishSensorData();
        lastPublishTime = millis();
    }
}
