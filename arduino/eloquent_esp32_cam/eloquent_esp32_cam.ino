#include <eloquent_esp32cam.h>

using eloq::camera;

void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("___GET YOUR FIRST PICTURE___");
    
    camera.pinout.aithinker();
    camera.brownout.disable();
    camera.resolution.vga();
    
    while (!camera.begin().isOk())
        Serial.println(camera.exception.toString());
}

void loop() {
    if (!camera.capture().isOk()) {
        Serial.println(camera.exception.toString());
        return;
    }
    
    Serial.printf(
        "JPEG size in bytes: %d. Width: %dpx. Height: %dpx.\n",
        camera.getSizeInBytes(),
        camera.resolution.getWidth(),
        camera.resolution.getHeight()
    );
}