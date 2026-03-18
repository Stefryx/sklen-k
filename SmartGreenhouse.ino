#include "DHT.h"
#include "webpage.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Servo.h>

// Nastavení WiFi Sítě
const char *ssid = "Google pixel 7a";
const char *password = "nevimcodelam";

// Definice Pinů Wemos D1 (ESP8266)
#define DHTPIN D1     // DHT11 Data pin
#define DHTTYPE DHT11 // DHT 11
#define SERVO_PIN D2  // Servo PWM pin
#define RELAY_PIN D3  // Relé modul pro čerpadlo
#define FLOAT_PIN D5  // Plovákový senzor vodní hladiny

// Multiplexer Piny
#define MUX_SIG A0    // Z pin z Multiplexeru jde do A0
#define MUX_S0 D4
#define MUX_S1 D6
#define MUX_S2 D7
#define MUX_S3 D8

// Globální Proměnné a Objekty
DHT dht(DHTPIN, DHTTYPE);
Servo windowServo;
ESP8266WebServer server(80);

// Stavové proměnné
float tempC = 0.0;
float humVzduch = 0.0;
float humPudyProc = 0.0;
bool isWaterLvlOk = true;
bool isWindowOpen = false;
bool isPumpRunning = false;
bool autoModeEnabled = true;

// Limity pro logiku automatu
const float TEMP_OPEN_WINDOW_THRESHOLD = 28.0;
const float TEMP_CLOSE_WINDOW_THRESHOLD = 25.0;
const int SOIL_DRY_THRESHOLD = 30;

// Časovací a bezpečnostní konstanty
unsigned long lastPumpRunTime = 0;
unsigned long lastPumpStartAttempt = 0;
const unsigned long PUMP_MANUAL_RUN_DURATION = 5000;
const unsigned long PUMP_COOLDOWN_DURATION = 60000;

// Funkce pro obsluhu Multiplexeru
void setMuxChannel(int channel) {
  digitalWrite(MUX_S0, bitRead(channel, 0));
  digitalWrite(MUX_S1, bitRead(channel, 1));
  digitalWrite(MUX_S2, bitRead(channel, 2));
  digitalWrite(MUX_S3, bitRead(channel, 3));
  delay(10);
}

int readSoilHumidity() {
  setMuxChannel(0);
  int rawAnalog = analogRead(MUX_SIG);
  int percentage = map(rawAnalog, 1024, 300, 0, 100);
  percentage = constrain(percentage, 0, 100);
  return percentage;
}

// Fyzické řídící funkce
void openWindow() {
  windowServo.write(180);
  isWindowOpen = true;
  Serial.println("[OKNO] Otevřeno");
}

void closeWindow() {
  windowServo.write(0);
  isWindowOpen = false;
  Serial.println("[OKNO] Zavřeno");
}

void toggleWindow() {
  if (isWindowOpen)
    closeWindow();
  else
    openWindow();
}

void startPump() {
  bool waterPresent = (digitalRead(FLOAT_PIN) == LOW);
  isWaterLvlOk = waterPresent;

  if (isWaterLvlOk) {
    digitalWrite(RELAY_PIN, LOW);
    isPumpRunning = true;
    lastPumpRunTime = millis();
    lastPumpStartAttempt = millis();
    Serial.println("[ČERPADLO] Spuštěno - Zahájeno zalévání");
  } else {
    stopPump();
    Serial.println("[VAROVÁNÍ] Voda v nádrži není! Čerpadlo NELZE spustit.");
  }
}

void stopPump() {
  digitalWrite(RELAY_PIN, HIGH);
  isPumpRunning = false;
  Serial.println("[ČERPADLO] Zastaveno");
}

// WebServer a REST API Routes
void handleRoot() { 
  server.send(200, "text/html", webpage_html); 
}

void handleApiData() {
  String json = "{";
  json += "\"temperature\":" + String(tempC) + ",";
  json += "\"humidity\":" + String(humVzduch) + ",";
  json += "\"soilHumidity\":" + String(humPudyProc) + ",";
  json += "\"waterLevelOk\":" + String(isWaterLvlOk ? "true" : "false") + ",";
  json += "\"pumpState\":" + String(isPumpRunning ? "true" : "false") + ",";
  json += "\"windowState\":" + String(isWindowOpen ? "true" : "false") + ",";
  json += "\"autoMode\":" + String(autoModeEnabled ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiPumpToggle() {
  if (isPumpRunning) {
    stopPump();
  } else {
    startPump();
  }
  server.send(200, "text/plain", "OK");
}

void handleApiWindowToggle() {
  toggleWindow();
  server.send(200, "text/plain", "OK");
}

void handleApiAutoModeToggle() {
  autoModeEnabled = !autoModeEnabled;
  Serial.println(autoModeEnabled ? "[REŽIM] Automatický mód AKTIVOVÁN" 
                                  : "[REŽIM] Automatický mód DEAKTIVOVÁN");
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("\n\n=== SmartGreenhouse Starting ===\n");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(FLOAT_PIN, INPUT_PULLUP);
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  pinMode(MUX_SIG, INPUT);

  stopPump();

  windowServo.attach(SERVO_PIN);
  closeWindow();

  dht.begin();
  Serial.println("[SENZOR] DHT inicializován");

  Serial.println();
  Serial.print("Připojování k WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int connectionAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 20) {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("✅ WiFi úspěšně připojena!");
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("❌ WiFi se nepodařilo připojit!");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/pump/toggle", HTTP_POST, handleApiPumpToggle);
  server.on("/api/window/toggle", HTTP_POST, handleApiWindowToggle);
  server.on("/api/automode/toggle", HTTP_POST, handleApiAutoModeToggle);

  server.begin();
  Serial.println("[SERVER] Webserver spuštěn!");
  Serial.println("=== Setup Hotov ===\n");
}

unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000;

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentMillis;

    tempC = dht.readTemperature();
    humVzduch = dht.readHumidity();
    humPudyProc = readSoilHumidity();
    isWaterLvlOk = (digitalRead(FLOAT_PIN) == LOW);

    Serial.print("[SENZORY] Teplota: ");
    Serial.print(tempC);
    Serial.print("°C | Vzdušná vlhkost: ");
    Serial.print(humVzduch);
    Serial.print("% | Půdní vlhkost: ");
    Serial.print(humPudyProc);
    Serial.print("% | Voda: ");
    Serial.println(isWaterLvlOk ? "OK" : "NÍZKO!");

    if (isnan(tempC) || isnan(humVzduch)) {
      Serial.println("[CHYBA] DHT senzor - Chyba čtení!");
    } else {
      if (autoModeEnabled) {
        if (tempC >= TEMP_OPEN_WINDOW_THRESHOLD && !isWindowOpen) {
          openWindow();
        } else if (tempC <= TEMP_CLOSE_WINDOW_THRESHOLD && isWindowOpen) {
          closeWindow();
        }

        if (humPudyProc < SOIL_DRY_THRESHOLD && isWaterLvlOk && !isPumpRunning) {
          if (lastPumpStartAttempt == 0 || 
              (currentMillis - lastPumpStartAttempt >= PUMP_COOLDOWN_DURATION)) {
            Serial.println("[ZALÉVÁNÍ] Půda je suchá - Spuštění čerpadla");
            startPump();
          } else {
            unsigned long remainingCooldown = PUMP_COOLDOWN_DURATION - 
                                               (currentMillis - lastPumpStartAttempt);
            Serial.print("[INFO] Čerpadlo v cooldown: ");
            Serial.print(remainingCooldown / 1000);
            Serial.println("s zbývá");
          }
        }
      }
    }
  }

  if (isPumpRunning) {
    if (digitalRead(FLOAT_PIN) == HIGH) {
      stopPump();
      Serial.println("[BEZPEČNOST] Voda skončila - Čerpadlo zastaveno!");
    }
    else if (currentMillis - lastPumpRunTime >= PUMP_MANUAL_RUN_DURATION) {
      stopPump();
      Serial.println("[BEZPEČNOST] Dosažen max čas - Čerpadlo zastaveno!");
    }
  }
}