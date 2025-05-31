
# ESP-IDF Projects

Based on Espressif, with ESP-IDF framework. Each project is configured to work with `idf.py` and multiple dependencies from Espressif. Make sure the ESP-IDF is correctly setup.

### Prerequisites

You should have:
- [ESP-IDF](https://github.com/espressif/esp-idf) (version 4.2 or higher, unless otherwsie mentioned in specific projects for backward compatibility.)
- Python >= 3.7
- Some Git version or flavor (Git for Windows not tested, but should work)
- CMake and Ninja build tools, are automatically available inside the cmd/powershell window of ESP-IDF. NOTE: Make sure you install the ESP-IDF tools, and you execute idf.py ONLY WITHIN THE PROVIDED TERMINAL WINDOWS.

### Initial Setup

1. Clone the ESP-IDF framework:
   ```bash
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh
   . ./export.sh
   ```

2. Go to the specific project folder within (multiple example folders exist).

3. Run the ESP-IDF menu configuration tool FOR EACH PROJECT. Multiple different configs might be needed to address project needs and issues (memory config, wifi, mqtt setup, etc.):
   ```bash
   idf.py menuconfig
   ```
   - Use to configure project-specifics, e.g., flash memory settings, PSRAM, Wi-Fi credentials, etc.
   - For certain projects (e.g., camera-based projects), enable PSRAM in the `menuconfig`, and set Flash and PSRAM frequencies to 80 MHz. More settings within the discussion.txt files inside specific project.
   - YOU NEED TO REDO THIS every time you add a new module, (e.g., esp-camera).

### Building and Flashing

1. Build project:
   ```bash
   idf.py build
   ```

2. Flash the firmware to your ESP device:
   ```bash
   idf.py -p COMXX flash
   ```
XX= com port.

3. Monitor the serial output:
   ```bash
   idf.py monitor
   # or combine the commands
   idf.py -p COMXX flash monitor
   ```

   Use `Ctrl+]` to exit the monitor.

### Adding Dependencies

Several projects require additional ESP-IDF components or libraries. To add them do:

- Use the `idf.py add-dependency`. For example:
  ```bash
  idf.py add-dependency "espressif/esp32-camera"
  ```
- Or directly add the dependency in the `idf_component.yml` file of the specific project.

Check the individual project's `CMakeLists.txt` for dependencies and configurations OF THE SPECIFIC PROJECT.

## Basic Projects included

### 1. ESP32-CAM (multiple versions)
- **Description**: Basic for the ESP32-CAM module for capturing images and streaming video. Start with this project to make sure that your camera is ok. When you first connect the camera, is better to test it with the arduino code in the folder IoT-Embedded/arduino/* .
- **Key Configurations**:
  - Make sure PSRAM is enabled.
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
  DONT FORGET the managed-components folder includes libraries from esp-df. In several cases in those files, debug messages have been inserted, look for "// George". ALL THOSE FILES ARE NOT UPDATED ANY MORE FROM GIT.

### 2. ESP32-S3 Modular Project App
- **Description**: Modular firmware for AWS IoT connectivity, including WIFI, MQTT communication and sensor integration.
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


For specific projects' descriptions and updated details and isntructions, check the readme.md files inside each subdirectory.


This project and its associated code are provided "AS IS", without any warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement. In no event shall the authors or contributors be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

This project is a personal adaptation and is not officially affiliated with or endorsed by Espressif Systems, the provider of the ESP32 hardware, ESP-IDF, or ESP-WHO frameworks.