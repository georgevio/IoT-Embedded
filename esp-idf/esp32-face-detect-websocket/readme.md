THIS is a face detection (not face identification) over the esp32-cam.

if you face problems, run the `idf.py menuconfig` again, although the sdkconfig holds the basic settings.

IMPORTANT: make sure that the /componets/esp-dl directory is from esp-who v1.1.0, this is the one containing the correct face recognition libraries. NEWER LIBRARIES DONT.