Read carefully the readme.md in the containing folder ../3-Level-Cloud for the full project architecture and description.

This here, is the edge cloud part of a three-tier cloud implementation.

In particular, the ESP32-S3 edge device performs facial recognition (identification) by comparing the received image against a local database of known faces. This intermediate step helps for rapid identification without cloud communication, reducing response time for standard, known faces (e.g., the database holds faces of all personnel).

If the face is not recognized in the local database, the ESP32-S3 transfers the task to the cloud tier via MQTT to send the image to AWS IoT for more intensive investigation, such as comparison against an extensive database or further analysis (e.g., false positives/negatives of sensors).