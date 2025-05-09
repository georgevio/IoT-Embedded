#ifndef BLESETUP_H
#define BLESETUP_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Config.h"

#define SERVER_NAME "ESP32-S3_BLE"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

void clearBondingInfo() {
  int numDevices = esp_ble_get_bond_device_num();
  esp_ble_bond_dev_t *devices = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * numDevices);
  esp_ble_get_bond_device_list(&numDevices, devices);
  //SERIAL_PRINT(F("Number of bonded devices: "));
  //SERIAL_PRINTLN(numDevices);
  for (int i = 0; i < numDevices; i++) {
    esp_ble_remove_bond_device(devices[i].bd_addr);
    const char configMessage[] PROGMEM = "Cleared bonded device ";
    SERIAL_PRINT(F(configMessage));
    SERIAL_PRINTLN(i + 1);
  }
  free(devices);
}

void setupBLE() { 
	clearBondingInfo(); 
	BLEDevice::init(SERVER_NAME); 
	pServer = BLEDevice::createServer(); 
	BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x181A)); pService->createCharacteristic(BLEUUID((uint16_t)0x2A6E), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); 
	pService->start(); 
	pServer->getAdvertising()->start(); 
	SERIAL_PRINTLN("BLE on");
}

#endif // BLESETUP_H
