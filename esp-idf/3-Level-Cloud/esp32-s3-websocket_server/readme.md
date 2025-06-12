**IMPORTANT:** Read carefully the readme.md in the containing folder ../3-Level-Cloud for the full project architecture and description.

This here, is the edge cloud part of a three-tier cloud implementation.

In particular, the ESP32-S3 edge device performs facial recognition (identification) by comparing the received image against a local database of known faces. This intermediate step helps for rapid identification without cloud communication, reducing response time for standard, known faces (e.g., the database holds faces of all personnel).

If the face is not recognized in the local database, the ESP32-S3 transfers the task to the cloud tier via MQTT to send the image to AWS IoT for more intensive investigation, such as comparison against an extensive database or further analysis (e.g., false positives/negatives of sensors).

## ESP-S3 Characteristics Discovery

Press BOTH buttons on the device, i.e., BOOT & RST. Run this

`esptool.py --port COM9 flash_id`

The output should be something like this. Note the memory, the chip freq, etc. 
```
esptool.py v4.8.1
Serial port COM9
Connecting...
Detecting chip type... ESP32-S3
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded PSRAM 8MB (AP_3v3)
Crystal is 40MHz
MAC: f0:9e:9e:20:9f:30
Uploading stub...
Running stub...
Stub running...
Manufacturer: 85
Device: 2018
Detected flash size: 16MB
Flash type set in eFuse: quad (4 data lines)
Flash voltage set by eFuse to 3.3V
Hard resetting via RTS pin...
```
