#include <HardwareSerial.h>

// Initialize hardware serial
HardwareSerial Serial1(1);

int flag=0;
int counter=0;

void setup() {
  if(flag==1){
    Serial1.begin(115200, SERIAL_8N1, 3, 1); // RX, TX pins
    Serial.begin(115200);
  }else{
    Serial.begin(115200);
  }
}

void loop() {
  if (flag==1){
      if (Serial1.available()) {
        String message = Serial1.readStringUntil('\n');
        Serial.println("Received: " + message);
      }
  }else{
    Serial.print("testing serial: ");
    counter++;
    Serial.println(counter);
  }

}
