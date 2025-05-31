
# ESP-IDF Projects

Projects designed to run on Espressif microcontrollers using the ESP-IDF framework. Each project is configured to work with `idf.py` and multiple dependencies from Espressif's libraries.

### Prerequisites

You should have the following installed on your system:
- [ESP-IDF](https://github.com/espressif/esp-idf) (version 4.2 or higher is recommended, unless specifically instructed inside subdirectories)
- Python 3.7 or later
- Git
- CMake and Ninja build tools, they will be automatically available inside the cmd/powershell window of ESP-IDF. NOTE: Make sure you install the ESP-IDF tools, and you execute idf.py ONLY WITHIN THE PROVIDED TERMINAL WINDOWS.

### Initial Setup

1. Clone the ESP-IDF framework if you haven't already:
   ```bash
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh
   . ./export.sh
   ```

2. Navigate to the specific project folder within this directory.

3. Run the ESP-IDF menu configuration tool:
   ```bash
   idf.py menuconfig
   ```
   - Use this tool to configure project-specific parameters, such as flash memory settings, PSRAM, Wi-Fi credentials, etc.
   - For certain projects (e.g., camera-based projects), enable PSRAM in the `menuconfig`, and set Flash and PSRAM frequencies to 80 MHz.
   - You might need to redo this, every time you add a new module, for example esp-camera.

### Building and Flashing

1. Build the project:
   ```bash
   idf.py build
   ```

2. Flash the firmware to your ESP device:
   ```bash
   idf.py flash
   ```

3. Monitor the serial output:
   ```bash
   idf.py monitor
   # or combine the commands
   idf.py -p COMXX flash monitor
   ```

   Use `Ctrl+]` to exit the monitor.

### Adding Dependencies

Several projects require additional ESP-IDF components or libraries. To add them do:

- Use the `idf.py add-dependency` command. For example:
  ```bash
  idf.py add-dependency "espressif/esp32-camera"
  ```
- Alternatively, manually include the dependency in the `idf_component.yml` file of the project.

Refer to the individual project's `CMakeLists.txt` files for specific dependencies and configurations.

## Projects Overview

### 1. ESP32-CAM
- **Description**: Basic for the ESP32-CAM module for capturing images and streaming video. Start with this project to make sure that your camera is ok. When you first connect the camera, is better to test it with the arduino code in the folder IoT-Embedded/arduino/* .
- **Key Configurations**:
  - Ensure PSRAM is enabled.
  - Include the `esp32-camera` driver in the project dependencies.
- **Project Structure**:
  ```
  esp32-cam/
  ├── main/                     # Main application code. 
  |   --secret.h                # insert the wifi credentials (gitignored)
  ├── managed_components/       # External libraries/components
  ├── sdkconfig.txt             # Default configuration file
  └── CMakeLists.txt            # Build configuration
  ```

### 2. ESP32-S3 Modular Firmware
- **Description**: Provides modular firmware for AWS IoT connectivity, including MQTT communication and sensor integration.
- **Key Features**:
  - MQTT communication module
  - BME280 sensor integration
- **Project Structure**:
  ```
  esp32-s3-wroom-1/
  ├── certificates/             # AWS IoT certificates (gitignored)
  ├── credentials/              # Sensitive configuration (gitignored)
  ├── main/                     # Main application code
  ├── sdkconfig.txt             # Default configuration file
  └── CMakeLists.txt            # Build configuration
  ```



