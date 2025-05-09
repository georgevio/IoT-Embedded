
# ESP-IDF Projects

This directory contains various projects designed to run on Espressif microcontrollers using the ESP-IDF framework. Each project is configured to work seamlessly with `idf.py` and incorporates dependencies from Espressif's libraries.

## Getting Started

To set up and run the projects in this directory, follow the steps below.

### Prerequisites

Ensure you have the following installed on your system:
- [ESP-IDF](https://github.com/espressif/esp-idf) (version 4.2 or higher is recommended)
- Python 3.7 or later
- Git
- CMake and Ninja build tools

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
   ```

   Press `Ctrl+]` to exit the monitor.

### Adding Dependencies

Some projects may require additional components or libraries. To add these dependencies:

- Use the `idf.py add-dependency` command. For example:
  ```bash
  idf.py add-dependency "espressif/esp32-camera"
  ```
- Alternatively, manually include the dependency in the `idf_component.yml` file of the project.

Refer to the individual project's `CMakeLists.txt` files for specific dependencies and configurations.

## Projects Overview

### 1. ESP32-CAM
- **Description**: This project enables the use of the ESP32-CAM module for capturing images and streaming video.
- **Key Configurations**:
  - Ensure PSRAM is enabled.
  - Include the `esp32-camera` driver in the project dependencies.
- **Project Structure**:
  ```
  esp32-cam/
  ├── main/                     # Main application code
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

### 3. ESP-Face-Rec
- **Description**: A facial recognition project for ESP32-S3 modules.
- **Key Configurations**:
  - Requires specific camera modules supported by the `esp32-camera` driver.
  - Certificates for secure communication.

## Configuration Details

### MenuConfig Parameters
- **PSRAM**:
  - Ensure PSRAM is enabled for projects requiring large memory (e.g., camera-based projects).
- **Wi-Fi and MQTT**:
  - Configure Wi-Fi credentials and MQTT settings in the `menuconfig` or relevant header files.
- **Debugging**:
  - Enable necessary debugging options for development and troubleshooting.

### Sensitive Information
Store sensitive information (e.g., Wi-Fi credentials, AWS IoT keys) in a secure, gitignored location such as `credentials/secrets.h`.

## Additional Notes

- Refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) for detailed instructions and troubleshooting.
- Ensure all dependencies are compatible with your ESP-IDF version.

---


