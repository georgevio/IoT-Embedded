# ESP32-CAM: Web-Based Face Detection with Terminal Output

## Overview

Web-based face detection using the ESP32-CAM module. You can view the live stream via a webpage, and detected faces. It also prints info about detected faces to the terminal.

This project is largely based on the original example from Espressif's ESP-WHO framework, with some modifications for debugging.

## Based On

* **ESP-IDF Framework** (v5.4.1)
* **ESP-WHO Framework, specifically version 1.1.0.** IMPORTANT: Newer versions do not seem to work with ESP32-CAM.
    The core logic is found in `human_face_detection/web` in ESP-WHO v1.1.0. This version has ESP32-CAM face detection.

## Features

Functionality taken from the original ESP-WHO example, with:

1.  **WiFi Credentials via `secret.h`:**
    * **Important:** `secret.h` is included in `.gitignore`. You need to create this file yourself with your WiFi details.
2.  **Terminal Output for Detections:**
    * When a face is detected, detailed information is printed to the terminal, including number of faces, confidence score (play with confidence numbers), category, and bounding box coordinates for each detected face.
    * Example Terminal Output:
        ```
        DEBUG HFD: Faces DETECTED! Attempting to print results...
        ---------------- PRINT DETECTION RESULTS --------------------
        INFO: Found 1 face(s) in this frame:
          Face 1:
            Score:    0.97
            Category: 1
            Box:      [x=42, y=68, w=119, h=163]
        ----------------------------------------------------------------
        ```
3.  **Additional Debug Messages:** extra `ESP_LOGI` messages have been added in the `app_main.cpp` and `wifi.c`.
4.  ** Extra Components:** Necessary components from ESP-WHO v1.1.0 (`modules/ai`, `modules/camera`, `modules/utility`, `esp-dl`, `fb_gfx`) in `components/` directory. The `modules/ai/who_human_face_detection.cpp` is altered for the terminal output.

## Setup and Usage

1.  **ESP-IDF Environment:** Make sure ESP-IDF environment is set up and initialized. USE ONLY THE TERMINALS PROVIDED BY IT.
2.  **Clone the Project:** Get the project files onto your local machine.
3.  **Create `main/secret.h`:**
    Navigate to the `main/` directory of this project and create a file named `secret.h`:
    ```c
    #ifndef SECRET_H_
    #define SECRET_H_

    #define WIFI_SSID "Your_WiFi_SSID"
    #define WIFI_PASSWORD "Your_WiFi_Password"

    #endif /* SECRET_H_ */
    ```
4.  **Configurations:**
    * Open an ESP-IDF Environment terminal in the project root, or navigate there.
    * Run `idf.py set-target esp32`.
    * Run `idf.py menuconfig`. Review the settings (especially `Serial Flasher Config`, `CPU Frequency (set to 240MHz)`, and `Component config -> ESP PSRAM`). Save and exit. NOTE: The project includes an `sdkconfig.defaults` file which should work.
5.  **Build:** `idf.py build`
6.  **Flash & Monitor:** `idf.py -p COMXX flash monitor` XX=com port.
7.  **Access Webpage:** After the devise boots and connects to WiFi, it will print the IP address in the terminal. Copy/Paste this IP address in a web browser. 

## Notes

* **Webpage Interaction:**  It currently detects faces and prints info to the terminal automatically. You may need to adjust settings in the webpage to start stream or start the "Face Detection" to enable green face detection boxes over the stream, as in the original project.
* **Camera Placement:** I noticed that the camera detects faces ONLY WHEN UPSIDE DOWN! So, try with the power/USB cable upside.

## Libraries and Versions

* **ESP-IDF:** (v5.4.1)
* **ESP-WHO:** Version 1.1.0 (components included locally, NOT CONNECTED TO GIT). Updates to these local components from newer ESP-WHO versions should be done with caution.
## Disclaimer

This project and its associated code are provided "AS IS", without any warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement. In no event shall the authors or contributors be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

This project is a personal adaptation and is not officially affiliated with or endorsed by Espressif Systems, the provider of the ESP32 hardware, ESP-IDF, or ESP-WHO frameworks.