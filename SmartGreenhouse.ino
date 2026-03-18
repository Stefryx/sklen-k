// Improved SmartGreenhouse.ino code

#include <WiFi.h>
#include <ArduinoJson.h>

// Constants
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Pin definitions
const int pumpPin = 5;

void setup() {
    Serial.begin(115200);
    setupWiFi();
    pinMode(pumpPin, OUTPUT);
}

void loop() {
    // Implement your logic here
}

void setupWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");
}

void startPump() {
    Serial.println("Starting pump...");
    digitalWrite(pumpPin, HIGH);
    delay(2000); // Run for 2 seconds
    Serial.println("Pump started safely.");
}

void stopPump() {
    Serial.println("Stopping pump...");
    digitalWrite(pumpPin, LOW);
    Serial.println("Pump stopped safely.");
}