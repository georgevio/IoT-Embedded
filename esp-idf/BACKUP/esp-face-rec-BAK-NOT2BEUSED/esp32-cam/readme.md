# ESP32-CAM IoT Project

This repository contains the source code for an ESP32-CAM based project. It includes functionalities for Wi-Fi connectivity, MQTT communication, and camera operation. There are also extensive debug messages, you may hide some.

## Overview

This project aims to create an application for the ESP32-CAM that can capture images and communicate them using the MQTT protocol, with a focus on secure communication using TLS/SSL certificates (not currently implemented).

## Files Included

The repository contains the following source and configuration files:

esp32-cam-project/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── wifi_connect.c
│   ├── mqtt_client.c
│   ├── camera_config.c
│   └── face_detect.c
├── components/
    └── image_util/
	
* **`main.c`**: The main application entry point and logic.
* **`wifi.c` / `wifi.h`**: Source and header files for managing Wi-Fi connection and related functionalities.
* **`mqtt.c` / `mqtt.h`**: Source and header files for handling MQTT communication, including connecting to a broker, publishing, and subscribing to topics.
* **`camera.c` / `camera.h`**: Source and header files for initializing and controlling the ESP32-CAM module.
* **`config.h`**: Header file likely containing project-specific configurations, such as Wi-Fi credentials, MQTT broker address, and camera settings.
* **`CMakeLists.txt`**: CMake build configuration file used by the ESP-IDF (Espressif IoT Development Framework).
* **`idf_component.yml`**: Component manifest file for ESP-IDF.
* **`certificates/`**: Directory containing the necessary SSL/TLS certificates for secure communication (likely with an MQTT broker like AWS IoT Core).
    * `AmazonRootCA1.pem`: Amazon Root CA certificate.
    * `new_certificate.pem`: Your device's certificate.
    * `new_private.key`: Your device's private key.

## Getting Started

### Prerequisites

* **ESP-IDF (Espressif IoT Development Framework):** Ensure you have the ESP-IDF installed and configured on your system. Follow the official ESP-IDF Getting Started guide for your operating system: [https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)
* **ESP32-CAM Module:** You will need an ESP32-CAM development board.
* **USB-to-Serial Adapter:** For flashing the firmware to the ESP32-CAM.
* **Wi-Fi Network:** Access to a Wi-Fi network for the ESP32-CAM to connect to.
* **MQTT Broker:** An MQTT broker to connect to (configured in `config.h`). If using the provided certificates, it's likely configured for AWS IoT Core.

### Configuration

1.  **Clone the Repository:**
    ```bash
    git clone <repository_url>
    cd <repository_name>
    git submodule update --init --recursive
    ```

2.  **Configure Project Settings:**
    * Open the `config.h` file and update the following configurations:
        * **Wi-Fi Credentials:** Set your Wi-Fi SSID and password.
        * **MQTT Broker Address:** Configure the address (and potentially port) of your MQTT broker.
        * **MQTT Topic:** Define the MQTT topics for publishing and subscribing.
        * **Camera Settings:** Adjust camera resolution, frame rate, etc., as needed.
        * **AWS IoT Core Endpoint (if applicable):** If using AWS IoT Core, ensure the endpoint in `config.h` matches your AWS IoT Core endpoint.

3.  **Configure ESP-IDF:**
    Navigate to the project directory in your terminal and run:
    ```bash
    idf.py menuconfig
    ```
    * Go to `Serial flasher config` and set the correct serial port for your ESP32-CAM.
    * Review other component configurations, especially under `Component config`:
        * **Wi-Fi:** Verify the Wi-Fi credentials.
        * **MQTT:** Check the MQTT client library configuration.
        * **ESP32-specific:** Ensure PSRAM is enabled if your ESP32-CAM board has it (recommended for camera usage).
        * **Camera Hardware:** Configure the camera interface settings, including the model (likely OV2640) and pin assignments according to your ESP32-CAM board.

4.  **Build the Project:**
    ```bash
    idf.py build
    ```

### Flashing the Firmware

1.  Connect your ESP32-CAM board to your computer using a USB-to-serial adapter. Ensure the board is in flashing mode (usually by holding the BOOT button while connecting and releasing shortly after).
2.  Flash the firmware using:
    ```bash
    idf.py -p <serial_port> flash
    ```
    Replace `<serial_port>` with the correct serial port of your ESP32-CAM.

### Monitoring the Output

You can monitor the serial output of the ESP32-CAM using:

```bash
idf.py -p <serial_port> monitor

Or combine to one command:

```bash
idf.py -p <serial_port> flash monitor




