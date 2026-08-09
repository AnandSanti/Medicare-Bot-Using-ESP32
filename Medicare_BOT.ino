
// ═══════════════════════════════════════════════════════════════════════════
// REQUIRED INCLUDES - ORDER MATTERS FOR ESP32
// ═══════════════════════════════════════════════════════════════════════════

#include <WiFi.h>                    // WiFi for ESP32
#include <WiFiClientSecure.h>        // Secure WiFi client (TLS/SSL)
#include <PubSubClient.h>            // MQTT client
#include <Wire.h>                    // I2C communication
#include "MAX30105.h"                // MAX30102 sensor library
#include "spo2_algorithm.h"          // SpO2 calculation
#include <ArduinoJson.h>             // JSON handling


// WiFi Configuration
const char* WIFI_SSID = "AnandSS";
const char* WIFI_PASSWORD = "1234567890";

// HiveMQ Cloud Configuration (Secure TLS/SSL)
const char* MQTT_BROKER = "b66615908ab34ddc896d02f156e83c29.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;                              // TLS/SSL Port
const char* MQTT_USER = "AnandSadashivSanti";
const char* MQTT_PASSWORD = "1234567890";
const char* MQTT_TOPIC = "Data/ESP/HealthMonitor";
const char* MQTT_CLIENT_ID = "ESP32-HealthMonitor";

// Pin Configuration
#define PIN_THERMISTOR 34           // ADC for thermistor
#define PIN_EMERGENCY_BTN 27        // Emergency button (INPUT_PULLUP)
#define PIN_BUZZER 26               // Buzzer/Speaker
#define PIN_LED 25                  // Status LED
#define I2C_SDA 21                  // MAX30102 SDA
#define I2C_SCL 22                  // MAX30102 SCL

// Thermistor Parameters (NTC 10K)
const float THERMISTOR_NOMINAL_RESISTANCE = 10000.0;
const float THERMISTOR_NOMINAL_TEMP = 298.15;
const float B_COEFFICIENT = 3950.0;
const float PULLUP_RESISTANCE = 10000.0;
const float SUPPLY_VOLTAGE = 3.3;

// Alert Thresholds
const int HEART_RATE_HIGH = 120;
const int HEART_RATE_LOW = 50;
const float TEMP_HIGH = 38.5;
const float TEMP_LOW = 35.0;
const int SPO2_LOW = 92;

// Timing
const unsigned long MQTT_INTERVAL = 5000;
const unsigned long SENSOR_INTERVAL = 1000;

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL VARIABLES
// ═══════════════════════════════════════════════════════════════════════════

MAX30105 maxSensor;
WiFiClientSecure espClient;         // Secure WiFi client for TLS/SSL
PubSubClient mqttClient(espClient);

// Sensor Data
float bodyTemp = 0.0;
int32_t heartRate = 0;
int8_t hrValid = 0;
int32_t spo2 = 0;
int8_t spo2Valid = 0;
uint32_t irBuffer[100];
uint32_t redBuffer[100];

// Status Flags
bool alertActive = false;
bool emergencyActive = false;
bool mqttConnected = false;
bool wifiConnected = false;

// Timestamps
unsigned long lastMqttPublish = 0;
unsigned long lastSensorRead = 0;

// ═══════════════════════════════════════════════════════════════════════════
// THERMISTOR TEMPERATURE READING
// ═══════════════════════════════════════════════════════════════════════════

float readTemperature() {
  int adcValue = analogRead(PIN_THERMISTOR);
  float voltage = adcValue * (SUPPLY_VOLTAGE / 4095.0);
  
  if (voltage >= SUPPLY_VOLTAGE || voltage <= 0) return 0.0;
  
  float resistance = (SUPPLY_VOLTAGE * PULLUP_RESISTANCE / voltage) - PULLUP_RESISTANCE;
  if (resistance <= 0) return 0.0;
  
  float logR = log(resistance / THERMISTOR_NOMINAL_RESISTANCE);
  float invT = (1.0 / THERMISTOR_NOMINAL_TEMP) + (logR / B_COEFFICIENT);
  float tempKelvin = 1.0 / invT;
  
  return tempKelvin - 273.15;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAX30102 SENSOR SETUP
// ═══════════════════════════════════════════════════════════════════════════

bool initializeMAX30102() {
  Serial.println("[MAX30102] Initializing...");
  
  // Use 400000 (400 kHz) directly instead of I2C_400kHz
  if (!maxSensor.begin(Wire, 400000)) {
    Serial.println("[ERROR] MAX30102 not found!");
    return false;
  }
  
  Serial.println("[OK] MAX30102 detected");
  maxSensor.setup(60, 4, 2, 100, 411, 4096);
  Serial.println("[OK] MAX30102 configured");
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAX30102 DATA READING
// ═══════════════════════════════════════════════════════════════════════════

void readMAX30102() {
  for (int i = 0; i < 100; i++) {
    while (!maxSensor.available()) {
      maxSensor.check();
      delay(1);
    }
    redBuffer[i] = maxSensor.getRed();
    irBuffer[i] = maxSensor.getIR();
    maxSensor.nextSample();
  }
  
  maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer, 
                                         &spo2, &spo2Valid, 
                                         &heartRate, &hrValid);
}

// ═══════════════════════════════════════════════════════════════════════════
// LED CONTROL
// ═══════════════════════════════════════════════════════════════════════════

void ledOn() {
  digitalWrite(PIN_LED, HIGH);
}

void ledOff() {
  digitalWrite(PIN_LED, LOW);
}

void ledBlink(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(duration);
    digitalWrite(PIN_LED, LOW);
    delay(duration);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// BUZZER CONTROL
// ═══════════════════════════════════════════════════════════════════════════

void buzzerBeep(int duration) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(duration);
  digitalWrite(PIN_BUZZER, LOW);
}

void buzzerAlert() {
  // Alert pattern: 2x beep
  buzzerBeep(500);
  delay(200);
  buzzerBeep(500);
}

void buzzerEmergency() {
  // Emergency pattern: continuous for 2 seconds
  digitalWrite(PIN_BUZZER, HIGH);
  delay(2000);
  digitalWrite(PIN_BUZZER, LOW);
}

// ═══════════════════════════════════════════════════════════════════════════
// ALERT CHECKING
// ═══════════════════════════════════════════════════════════════════════════

bool checkAlerts() {
  bool alert = false;
  
  // Check Heart Rate
  if (hrValid) {
    if (heartRate > HEART_RATE_HIGH) {
      Serial.print("[ALERT] HR HIGH: ");
      Serial.print(heartRate);
      Serial.println(" BPM");
      alert = true;
    }
    else if (heartRate < HEART_RATE_LOW) {
      Serial.print("[ALERT] HR LOW: ");
      Serial.print(heartRate);
      Serial.println(" BPM");
      alert = true;
    }
  }
  
  // Check Temperature
  if (bodyTemp > TEMP_HIGH) {
    Serial.print("[ALERT] TEMP HIGH: ");
    Serial.print(bodyTemp, 2);
    Serial.println("°C");
    alert = true;
  }
  else if (bodyTemp < TEMP_LOW) {
    Serial.print("[ALERT] TEMP LOW: ");
    Serial.print(bodyTemp, 2);
    Serial.println("°C");
    alert = true;
  }
  
  // Check SpO2
  if (spo2Valid && spo2 < SPO2_LOW) {
    Serial.print("[ALERT] SpO2 LOW: ");
    Serial.print(spo2);
    Serial.println("%");
    alert = true;
  }
  
  return alert;
}

// ═══════════════════════════════════════════════════════════════════════════
// WIFI INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

void initializeWiFi() {
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("[OK] WiFi connected!");
    Serial.print("     IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("[ERROR] WiFi failed!");
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// MQTT CONNECTION
// ═══════════════════════════════════════════════════════════════════════════

void connectMQTT() {
  int retries = 0;
  
  Serial.println("[MQTT] Connecting to HiveMQ Cloud...");
  espClient.setInsecure();  // Skip certificate validation for testing
  
  while (!mqttClient.connected() && retries < 10) {
    Serial.print("[MQTT] Attempt ");
    Serial.print(retries + 1);
    Serial.println("/10");
    
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("[OK] Connected to HiveMQ Cloud!");
      mqttConnected = true;
      ledBlink(3, 100);  // 3 quick blinks on connection
      return;
    } else {
      Serial.print("[ERROR] Code: ");
      Serial.println(mqttClient.state());
      retries++;
      delay(5000);
    }
  }
  
  mqttConnected = false;
  Serial.println("[ERROR] MQTT connection failed");
}

// ═══════════════════════════════════════════════════════════════════════════
// MQTT DATA PUBLISHING
// ═══════════════════════════════════════════════════════════════════════════

void publishData() {
  if (!mqttConnected) return;
  
  DynamicJsonDocument doc(256);
  
  doc["bodyTemp"] = round(bodyTemp * 100) / 100.0;
  doc["spo2"] = spo2Valid ? spo2 : 0;
  doc["heartRate"] = hrValid ? heartRate : 0;
  doc["alert"] = alertActive;
  doc["emergency"] = emergencyActive;
  doc["timestamp"] = millis();
  doc["device"] = "ESP32-HealthMonitor";
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  if (mqttClient.publish(MQTT_TOPIC, jsonPayload.c_str())) {
    Serial.print("[MQTT] Published: ");
    Serial.println(jsonPayload);
  } else {
    Serial.println("[ERROR] MQTT publish failed");
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP FUNCTION
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Print Startup Banner
  Serial.println("\n\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║          HEALTH MONITOR - STARTING UP                ║");
  Serial.println("║                                                            ║");
  Serial.println("║  MAX30102 + Thermistor + LED + Buzzer + MQTT              ║");
  Serial.println("║  HiveMQ Cloud (TLS/SSL) - Secure Monitoring               ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝\n");
  
  // Initialize I2C
  Serial.println("[INIT] Starting I2C...");
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  Serial.println("[OK] I2C initialized");
  
  // Initialize GPIO
  Serial.println("[INIT] Configuring GPIO...");
  pinMode(PIN_THERMISTOR, INPUT);
  pinMode(PIN_EMERGENCY_BTN, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);
  Serial.println("[OK] GPIO configured");
  
  // Initialize MAX30102
  if (!initializeMAX30102()) {
    Serial.println("[CRITICAL] MAX30102 failed!");
    ledBlink(10, 100);  // Error blink pattern
    while (1) delay(1);
  }
  
  // Initialize WiFi
  initializeWiFi();
  
  // Initialize MQTT
  if (wifiConnected) {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    connectMQTT();
  }
  
  Serial.println("\n[OK] Setup complete! Starting monitoring...\n");
  Serial.println("═══════════════════════════════════════════════════════════");
  
  lastSensorRead = millis();
  lastMqttPublish = millis();
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN LOOP
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
  unsigned long now = millis();
  
  // WiFi Reconnect
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    Serial.println("[WARNING] WiFi lost, reconnecting...");
    initializeWiFi();
  }
  
  // MQTT Reconnect
  if (wifiConnected) {
    if (!mqttClient.connected()) {
      Serial.println("[WARNING] MQTT disconnected, reconnecting...");
      connectMQTT();
    }
    mqttClient.loop();
  }
  
  // ─── Read Sensors ─────────────────────────────────────────────────────
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    
    // Read Temperature
    bodyTemp = readTemperature();
    
    // Read Heart Rate & SpO2
    readMAX30102();
    
    // Print Readings
    Serial.println("\n┌─ SENSOR READINGS ────────────────────────────");
    Serial.print("│ Temperature:  ");
    Serial.print(bodyTemp, 2);
    Serial.println(" °C");
    
    if (hrValid) {
      Serial.print("│ Heart Rate:   ");
      Serial.print(heartRate);
      Serial.println(" BPM");
    } else {
      Serial.println("│ Heart Rate:   [No reading]");
    }
    
    if (spo2Valid) {
      Serial.print("│ SpO2:         ");
      Serial.print(spo2);
      Serial.println(" %");
    } else {
      Serial.println("│ SpO2:         [No reading]");
    }
    
    Serial.println("└──────────────────────────────────────────────");
  }
  
  // ─── Check Emergency Button ───────────────────────────────────────────
  if (digitalRead(PIN_EMERGENCY_BTN) == LOW) {
    Serial.println("\n🚨 EMERGENCY BUTTON PRESSED! 🚨\n");
    emergencyActive = true;
    buzzerEmergency();
    ledOn();
  } else {
    emergencyActive = false;
    ledOff();
  }
  
  // ─── Check Alerts ─────────────────────────────────────────────────────
  alertActive = checkAlerts();
  if (alertActive) {
    buzzerAlert();
    ledBlink(2, 200);
  }
  
  // ─── Publish to MQTT ──────────────────────────────────────────────────
  if (now - lastMqttPublish >= MQTT_INTERVAL) {
    lastMqttPublish = now;
    
    if (wifiConnected && mqttConnected) {
      publishData();
    } else {
      Serial.println("[INFO] MQTT unavailable, skipping publish");
    }
  }
  
  delay(100);
}


