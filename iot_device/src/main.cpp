#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h> 

// MQTT Config
const char* MQTT_BROKER = "192.168.29.98";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "mps-bam100";
const char* MQTT_PASS = "bam100";

// Topics
const String DEVICE_ID = "IoT_Device_" + String((uint32_t)ESP.getEfuseMac(), HEX);
const String CMD_TOPIC = DEVICE_ID + "/cmd";
const String STATUS_TOPIC = DEVICE_ID + "/status";

// Pins
#define LED_PIN 2      // Built-in LED
#define STATUS_LED 13  // Additional status LED

WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiManager wifiManager;

//=======================
// Function Declarations (Forward Declare)
//=======================
void startWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);

//=======================
// Setup
//=======================

void setup() {
  Serial.begin(9600);
  Serial.println("Firmware v1.1 - OTA Enabled");
  pinMode(LED_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  
  startWiFi();
  Serial.println("Device ID: " + DEVICE_ID); 
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

    // ========== OTA Setup ==========
    ArduinoOTA.setHostname(DEVICE_ID.c_str()); // Use device ID as hostname
  
    ArduinoOTA.onStart([]() {
      Serial.println("OTA Update Started");
      digitalWrite(LED_PIN, LOW); // Turn off LED during update
    });
  
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA Update Complete!");
    });
  
    ArduinoOTA.onProgress([](int progress, int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

//=======================
// Main Loop
//=======================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(STATUS_LED, LOW);
    startWiFi();
  } 
  else if (!mqtt.connected()) {
    reconnectMQTT();
  } 
  else {
    digitalWrite(STATUS_LED, HIGH);
    mqtt.loop();
  }

  ArduinoOTA.handle(); // Must be called regularly to handle OTA updates
  delay(10); // Add a small delay to avoid blocking the loop
}

void publishDiscoveryConfig() {
  String config = R"({
    "name":"IoT_Device_2265b7a0",
    "cmd_t":"IoT_Device_2265b7a0/cmd",
    "stat_t":"IoT_Device_2265b7a0/status",
    "avty_t":"IoT_Device_2265b7a0/online",
    "pl_on":"ON",
    "pl_off":"OFF"
  })";
  
  mqtt.publish("homeassistant/light/2265b7a0/config", config.c_str());
}


// Add this right after MQTT connects successfully
void setupHADiscovery() {
  JsonDocument doc; 
  doc["name"] = "ESP32 Light";
  doc["unique_id"] = DEVICE_ID;
  doc["cmd_t"] = CMD_TOPIC.c_str();
  doc["stat_t"] = STATUS_TOPIC.c_str();
  doc["schema"] = "json";
  doc["brightness"] = false;
  doc["retain"] = true;

  String discoveryTopic = "homeassistant/light/" + DEVICE_ID + "/config";
  String payload;
  serializeJson(doc, payload);

  mqtt.publish(discoveryTopic.c_str(), payload.c_str(), true); // Retained message
  Serial.println("Published HA discovery config");
}

//=======================
// WiFi Connection
//=======================
void startWiFi() {
  Serial.println("\nConnecting to WiFi...");
  
  WiFi.mode(WIFI_STA);
  if (!wifiManager.autoConnect("IoT_Device_AP")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();
    delay(1000);
  }
  
  Serial.println("WiFi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());
}

//=======================
// MQTT Functions
//=======================
void reconnectMQTT() {
  Serial.print("Connecting to MQTT...");
  
  if (mqtt.connect(DEVICE_ID.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("connected!");
    publishDiscoveryConfig(); // Publish discovery config
    setupHADiscovery(); // Setup Home Assistant discovery
    mqtt.publish(STATUS_TOPIC.c_str(), "OFF", true); // Retained message
    mqtt.subscribe(CMD_TOPIC.c_str());
    mqtt.publish(STATUS_TOPIC.c_str(), "online");
  } 
  else {
    Serial.print("failed, rc=");
    Serial.print(mqtt.state());
    Serial.println(" retrying in 5s");
    delay(5000);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "ON") {
    digitalWrite(LED_PIN, HIGH);
    mqtt.publish(STATUS_TOPIC.c_str(), "ON");
  } 
  else if (message == "OFF") {
    digitalWrite(LED_PIN, LOW);
    mqtt.publish(STATUS_TOPIC.c_str(), "OFF");
  }
}