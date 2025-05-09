int counter = 0;

void setup() {
  // Start serial communication at 115200 baud rate
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 16, 17); // RX, TX pins
}

void loop() {
   Serial.println("Sending Hello ESP32-CAM");
  // Send a message to the ESP32-CAM
  Serial1.println("Hello ESP32-CAM");
  delay(10000); // Wait for a second
  counter++;
  Serial.print("Round: ");
  Serial.println(counter);

}
