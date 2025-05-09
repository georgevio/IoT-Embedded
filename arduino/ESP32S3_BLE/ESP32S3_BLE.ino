#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE Server Name
#define SERVER_NAME "ESP32-S3_BLE"

// Create BLE Server
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

// Function to clear bonding information
void clearBondingInfo() {
  int numDevices = esp_ble_get_bond_device_num();
  esp_ble_bond_dev_t *devices = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * numDevices);
  esp_ble_get_bond_device_list(&numDevices, devices);
  Serial.printf("Number of bonded devices: %d\n", numDevices);
  for (int i = 0; i < numDevices; i++) {
    esp_ble_remove_bond_device(devices[i].bd_addr);
    Serial.printf("Cleared bonded device %d\n", i+1);
  }
  free(devices);
}

// Event handler for BLE security events
void gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch (event) {
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
      Serial.println("Authentication complete.");
      break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
      Serial.printf("The passkey Notify number: %06d\n", param->ble_security.key_notif.passkey);
      break;
    case ESP_GAP_BLE_KEY_EVT:
      Serial.println("Key event");
      break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
      Serial.println("Security request received");
      break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
      Serial.println("Passkey request");
      break;
    case ESP_GAP_BLE_NC_REQ_EVT:
      Serial.println("Numeric Comparison request");
      break;
    default:
      Serial.printf("Unhandled GAP event: %d\n", event);
      break;
  }
}

// Register the GAP event handler
void registerGapHandler() {
  esp_ble_gap_register_callback(gapEventHandler);
}

void setup() {
  Serial.begin(115200);
  delay(500); // Add a delay to ensure Serial Monitor initializes
  Serial.println("Serial port started at 115200");
  
  // Clear previous bonding information
  clearBondingInfo();
  
  // Initialize BLE
  BLEDevice::init(SERVER_NAME);
  pServer = BLEDevice::createServer();
  
  // Create BLE Service
  BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x181A));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      BLEUUID((uint16_t)0x2A6E),
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  
  Serial.println("Bluetooth server started. Connect using a mobile device.");
  
  // Register GAP event handler
  registerGapHandler();
}

void loop() {
  delay(1000); // Delay to prevent flooding the loop
}
