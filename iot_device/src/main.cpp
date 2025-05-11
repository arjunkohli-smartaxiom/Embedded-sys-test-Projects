#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Preferences.h>
Preferences preferences;
// MQTT Config
// const char* MQTT_BROKER = "192.168.29.98";
// const int MQTT_PORT = 1883;
// const char* MQTT_USER = "mps-bam100";
// const char* MQTT_PASS = "bam100";

const char* MQTT_BROKER = "gull.rmq.cloudamqp.com";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "ejumsfuq:ejumsfuq";
const char* MQTT_PASS = "23apT7-ha1RDMnhhjNOSPUYlCcXZeURj";

// Device Info
const String DEVICE_ID = "IoT_Device_" + String((uint32_t)ESP.getEfuseMac(), HEX);
const String CMD_TOPIC = DEVICE_ID + "/cmd";
const String STATUS_TOPIC = DEVICE_ID + "/status";

#define LED_PIN 2
#define RESET_PIN 13   // BOOT button is GPIO0 on most ESP32 boards

WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiManager wifiManager;



WiFiManagerParameter custom_email("email", "Enter your email", "", 40);

bool mqttRegistered = false;
unsigned long lastReconnectAttempt = 0;

void reconnect() {
  if (mqtt.connect(DEVICE_ID.c_str(), MQTT_USER, MQTT_PASS)) {
    mqtt.subscribe(CMD_TOPIC.c_str());
    mqtt.publish(STATUS_TOPIC.c_str(), "OFF", true);
    Serial.println("MQTT Connected!");
  }
}


String email = "";
void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  // 1. Handle Reset Button at Boot
  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("[RESET] Clearing WiFi & Email...");
    digitalWrite(LED_PIN, HIGH);
    wifiManager.resetSettings();
    preferences.begin("iot-config", false);
    preferences.clear();
    preferences.end();
    delay(3000);
    digitalWrite(LED_PIN, LOW);
  }

  // 2. Load Saved Email
  preferences.begin("iot-config", true);
  email = preferences.getString("email", "");
  preferences.end();
  Serial.print("Stored Email: ");
  Serial.println(email.isEmpty() ? "<none>" : email);

  // 3. WiFi Setup (Non-Blocking)
  WiFi.mode(WIFI_STA);
  if (email.isEmpty()) {
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.addParameter(&custom_email);
    wifiManager.startConfigPortal("IoT_Device_AP");
    Serial.println("Enter Config Portal!");
  } else {
    WiFi.begin();
  }
  
  // 4. MQTT Setup
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
    String msg;
    for(int i=0; i<length; i++) msg += (char)payload[i];
    msg.trim();
    digitalWrite(LED_PIN, msg == "ON" ? HIGH : LOW);
    mqtt.publish(STATUS_TOPIC.c_str(), msg.c_str(), true);
    Serial.print("CMD: ");
    Serial.println(msg);
  });
}

void loop() {
  // 1. Handle Config Portal
  if (email.isEmpty()) {
    wifiManager.process();
    email = custom_email.getValue();
    if (!email.isEmpty()) {
      preferences.begin("iot-config", false);
      preferences.putString("email", email);
      preferences.end();
      Serial.print("Email Saved: ");
      Serial.println(email);
      WiFi.mode(WIFI_STA);
      WiFi.begin();
    }
    return;
  }

  // 2. Handle WiFi Connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
    return;
  }

  // 3. Handle MQTT
  if (!mqtt.connected()) {
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      Serial.println("Reconnecting MQTT...");
      reconnect();
    }
  } else {
    mqtt.loop();
  }

  // 4. Register Device Once
  if (!mqttRegistered && mqtt.connected()) {
    String payload = "{\"email\":\"" + email + "\",\"deviceId\":\"" + DEVICE_ID + "\"}";
    if (mqtt.publish("devices/register", payload.c_str())) {
      Serial.println("Device Registered!");
      mqttRegistered = true;
    }
  }
}