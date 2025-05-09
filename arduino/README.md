# ESP32 Experimental Projects

![GitHub last commit](https://img.shields.io/github/last-commit/georgevio/embedded)
![GitHub repo size](https://img.shields.io/github/repo-size/georgevio/embedded)
![GitHub issues](https://img.shields.io/github/issues/georgevio/embedded)

#esp32 | #esp32-wroom-32 | #espcam

Welcome to experimental code repository focused on the ESP32 family of microcontrollers, including the ESP32-WROOM-32 and ESP-CAM modules.

## Project Overview

This repository contains a collection of experimental code for the ESP32 family. The primary goal of this project is to develop an embedded AI application capable of facial recognition. The application aims to identify faces and crawl the internet for similar images or faces.

## Features

- **ESP32 Microcontroller**: Utilized for its powerful processing capabilities and low power consumption.
- **ESP-CAM Module**: Integrated camera module for capturing images.
- **Facial Recognition**: Implementing AI algorithms to recognize and match faces.
- **Web Crawling**: Searching the web for similar images or faces.

## Requirements

- **Hardware**:
  - ESP32 Development Board (e.g., ESP32-WROOM-32)
  - ESP-CAM Module
  - Power Supply
  - Cables and Connectors

- **Software**:
  - Arduino IDE
  - ESP32 Board Package for Arduino
  - OpenCV or similar library for AI and facial recognition
  - Python (for web crawling)

## Installation and Setup

1. **Set Up Arduino IDE**:
   - Install the Arduino IDE from [Arduino's official website](https://www.arduino.cc/en/software).
   - Add the ESP32 board support to Arduino IDE. Follow the instructions [here](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/).

2. **Connect to Wi-Fi**:
   - Create a file "secrtets.h" inside the project's folder. This file name is already in the .gitignore
   - Insert into the file the following code (Change your wifi SSID and password)
     ```cpp
        #pragma once
        #define WIFI_SSID "YourSSID"
        #define WIFI_PASSWORD "YourSecretPassword"

2. **Connect ESP-CAM Module**:
   - Connect the ESP-CAM to your ESP32 development board using the appropriate GPIO pins.
   - Power up the module and ensure the connections are secure.

3. **Upload Sample Code**:
   - Open the Arduino IDE, select the correct board and port.
   - Upload the sample code provided in the `examples/` directory of this repository.

4. **Run Facial Recognition Algorithm**:
   - Use the provided Python scripts or OpenCV to process images and identify faces.
   - Execute the web crawling script to search for similar images or faces on the internet.

## Contributing

Contributions are welcome! Please fork this repository and submit a pull request with your improvements or new experimental code. Ensure all tests pass before submitting your pull request.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.

## Note

This is a personal project, provided "as is," without any warranty or guarantee.

## Contact

For any questions or suggestions, feel free to contact me through GitHub Issues or by email at [georgevio@gmail.com](mailto:georgevio@gmail.com).
