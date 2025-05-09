#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "Casa Violetta";
const char* password = "$$123789$$";

// Create a web server object
WebServer server(80);

// Function to handle the root endpoint
void handleRoot() {
  server.send(200, "text/html", "<h1>Hello, this is ESP32-S3!</h1>");
}

// Function to handle the /status endpoint and return JSON
void handleStatus() {
  StaticJsonDocument<200> doc;
  doc["status"] = "OK";
  doc["uptime"] = millis() / 1000;  // Uptime in seconds

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Function to handle the /sensor endpoint and return JSON
void handleSensor() {
  StaticJsonDocument<200> doc;
  doc["temperature"] = 25.6;  // Example temperature value
  doc["humidity"] = 60.2;     // Example humidity value

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Function to handle unknown routes
void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Configure the web server routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/sensor", handleSensor);
  server.onNotFound(handleNotFound);
  
  // Start the web server
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Handle client requests
  server.handleClient();
}
