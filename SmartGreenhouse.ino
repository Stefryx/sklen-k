#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include "DHT.h"
#include "webpage.h"

// Nastavení WiFi Sítě

const char* ssid = "TVOJE_WIFI_JMENO";
const char* password = "TVOJE_WIFI_HESLO";

// Definice Pinů Wemos D1 (ESP8266)

#define DHTPIN D1       // DHT11 Data pin
#define DHTTYPE DHT11   // DHT 11
#define SERVO_PIN D2    // Servo PWM pin
#define RELAY_PIN D3    // Relé modul pro čerpadlo
#define FLOAT_PIN D4    // Plovákový senzor vodní hladiny

// Multiplexer Piny
#define MUX_SIG A0      // Z pin z Multiplexeru jde do A0
#define MUX_S0 D5       
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
int humPudyProc = 0.0;
bool isWaterLvlOk = true;

bool isWindowOpen = false;
bool isPumpRunning = false;
bool autoModeEnabled = true;

// Limity pro logiku automatu
const float TEMP_OPEN_WINDOW_THRESHOLD = 28.0; // °C pro otevření okna
const float TEMP_CLOSE_WINDOW_THRESHOLD = 25.0; // °C pro zavření
const int SOIL_DRY_THRESHOLD = 30; // % vlhkosti půdy pro zahájení zalití

unsigned long lastPumpRunTime = 0;
const unsigned long PUMP_MANUAL_RUN_DURATION = 5000; // Manuální spuštění na 5s 

// Funkce pro obsluhu Multiplexeru

void setMuxChannel(int channel) {
  // Převede číslo 0-15 na binární sekvenci pinů S0-S3
  digitalWrite(MUX_S0, bitRead(channel, 0));
  digitalWrite(MUX_S1, bitRead(channel, 1));
  digitalWrite(MUX_S2, bitRead(channel, 2));
  digitalWrite(MUX_S3, bitRead(channel, 3));
  delay(10);
}

int readSoilHumidity() {
  setMuxChannel(0); // vlhkost půdy máme na pinu Y0 na MUXu
  int rawAnalog = analogRead(MUX_SIG);
  
  // Transformace na procenta 
  // 1024 (na suchu) a kdy 400 (celé ve vodě).
  // ESP8266 A0 většinou dá 0-1024. Senzor ve vzduchu dává spíše víc, ve vodě méně.
  int percentage = map(rawAnalog, 1024, 300, 0, 100); 
  percentage = constrain(percentage, 0, 100);
  return percentage;
}


// Fyzické řídící funkce

void openWindow() {
  windowServo.write(180); // Plně otevřeno
  isWindowOpen = true;
}

void closeWindow() {
  windowServo.write(0); // Zavřeno
  isWindowOpen = false;
}

void toggleWindow() {
  if (isWindowOpen) closeWindow(); else openWindow();
}

void startPump() {
  // Před spuštěním ověříme plovák
  isWaterLvlOk = digitalRead(FLOAT_PIN); 
  if (isWaterLvlOk) {
    digitalWrite(RELAY_PIN, LOW);
    isPumpRunning = true;
    lastPumpRunTime = millis();
  } else {
    // Nádrž je prázdná, zabrání spuštění.
    stopPump();
  }
}

void stopPump() {
  digitalWrite(RELAY_PIN, LOW);
  isPumpRunning = false;
}


// WebServer a REST API Routs

void handleRoot() {
  server.send(200, "text/html", webpage_html);
}

void handleApiData() {
  // Poslat JSON strukturovaná data klientskému prohlížeči
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
  if(isPumpRunning) { stopPump(); }
  else { startPump(); }
  server.send(200, "text/plain", "OK");
}

void handleApiWindowToggle() {
  toggleWindow();
  server.send(200, "text/plain", "OK");
}

void handleApiAutoModeToggle() {
  autoModeEnabled = !autoModeEnabled;
  server.send(200, "text/plain", "OK");
}



// Arduino INIT

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Pin modes
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(FLOAT_PIN, INPUT_PULLUP); // Použije vestavěný rezistor pro plovák
  
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  pinMode(MUX_SIG, INPUT);
  
  stopPump(); // Pojistka off při startu
  
  // Servo Initialization
  windowServo.attach(SERVO_PIN);
  closeWindow(); // Výchozí stav
  
  // DHT Init
  dht.begin();
  
  // --------- WIFI CONNECTION ---------
  Serial.println();
  Serial.print("Připojování k síti WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // Čekání na spojení
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("🏆 WiFi úspěšně Připojena");
  Serial.print("IP adresa pro webový prohlížeč: ");
  Serial.println(WiFi.localIP());

  // --------- WEBSERVER ROUTES ---------
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/pump/toggle", HTTP_POST, handleApiPumpToggle);
  server.on("/api/window/toggle", HTTP_POST, handleApiWindowToggle);
  server.on("/api/automode/toggle", HTTP_POST, handleApiAutoModeToggle);
  
  server.begin();
  Serial.println("Server spuštěn!");
}


// Hlavní Smyčka

unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000;

void loop() {
  server.handleClient();
  
  // 1. Vždy po SENSOR_READ_INTERVAL chceme zaktualizovat stavy a tvořit logiku
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentMillis;
    
    // Čtení senzorů
    tempC = dht.readTemperature();
    humVzduch = dht.readHumidity();
    humPudyProc = readSoilHumidity();
    isWaterLvlOk = digitalRead(FLOAT_PIN);

    // Ignoruj chybné čtení DHT
    if (isnan(tempC) || isnan(humVzduch)) {
      Serial.println("Chyba čtení z DHT senzoru!");
    } else {
      // 2. AUTOMATICKÁ LOGIKA (pokud je webem povolena)
      if (autoModeEnabled) {
        
        // Větrání Logika
        if (tempC >= TEMP_OPEN_WINDOW_THRESHOLD && !isWindowOpen) {
          openWindow();
        } else if (tempC <= TEMP_CLOSE_WINDOW_THRESHOLD && isWindowOpen) {
          closeWindow();
        }
        
        // Zavlažování Logika
        if (humPudyProc < SOIL_DRY_THRESHOLD && isWaterLvlOk && !isPumpRunning) {
          // Příliš sucho -> Zalije
          startPump();
        } 
      }
    }
  }
  
  // 3. Bezpečnostní a časový STOP čerpadla
  if (isPumpRunning) {
     // Pokud došla voda během zalévání okamžitě vyplne 
     // NEBO běží již déle, než je povoleno (PUMP_MANUAL_RUN_DURATION)
     if (!digitalRead(FLOAT_PIN) || (currentMillis - lastPumpRunTime >= PUMP_MANUAL_RUN_DURATION)) {
       stopPump();
     }
  }
}
