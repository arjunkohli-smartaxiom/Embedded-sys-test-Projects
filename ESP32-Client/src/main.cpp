
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "password.h"

// MQTT Broker Details
//const char* mqtt_server = "192.168.29.193";
//const char* mqtt_server = "192.168.29.25"; // hrishita
const char* mqtt_server = "192.168.29.98";
const int mqtt_port = 1883;
const char* mqtt_user = "mps-bam100";
const char* mqtt_password = "bam100";

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);

// Device Details

//String device_id = "ESP32-" + String(ESP.getEfuseMac(), HEX);
String device_id = "123456";
const char* device_serial = "234AM87695";
const char* firmware_version = "2.01";

// Timer for Ping Interval
unsigned long lastPingTime = 0;
const unsigned long pingInterval = 30000;

// LED & Shade Control Pins
const int ledPins[] = {2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32};
const int shadePins[] = {33, 34, 35, 36};

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
// Function to handle the reboot command
void handleRebootCommand(const JsonDocument& doc) {
  if (doc.containsKey("deviceId") && doc["deviceId"] == "reboot") {
    Serial.println("🔄 Rebooting ESP32...");
    delay(1000); // Short delay before reboot
    esp_restart(); // Reboot the ESP32
  }
}
void sendDeviceDiscovery() {
  JsonDocument doc; // Use JsonDocument instead of StaticJsonDocument
  doc["device_id"] = device_id;
  doc["SNO"] = "234AM87696";
  doc["Firmware"] = "v1.0.0.1";
  doc["MacAddr"] = WiFi.macAddress();

  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("MPS/global/discovery", buffer, true);
  Serial.println("📢 Published Discovery Data:");
  Serial.println(buffer);
}
void sendConfigResponse() {
  StaticJsonDocument<256> doc;
  doc["ch_t"] = "LED";
  doc["ch_addr"] = "LED1";
  doc["cmd"] = 100; // Replace 100 with your actual deviceConfig.LED_CONFIG value
  doc["cmd_m"] = "config";

  char buffer[256];
  serializeJson(doc, buffer);
  String topic = String("MPS/global/") + device_id + "/config";
  client.publish(topic.c_str(), buffer);
  Serial.println("📤 Sent Config Response:");
  Serial.println(buffer);
}


void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("🔄 Connecting to MQTT broker...");
    if (client.connect(device_id.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("✅ MQTT connected");
            
      // Subscriptions
      client.subscribe((String("MPS/global/") + device_id + "/config").c_str());
      client.subscribe((String("MPS/global/") + device_id + "/control").c_str());
      client.subscribe((String("MPS/global/") + device_id + "/reboot").c_str());
      String sceneTopic = "MPS/global/" + device_id + "/scene";
      client.subscribe(sceneTopic.c_str());
      Serial.println("🎨 Subscribed to Scene Topic: " + sceneTopic);
      Serial.println("📡 Subscribed to Control Topics");
      sendDeviceDiscovery();
    } else {
      Serial.print("❌ MQTT Connection Failed. State: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}


void sendStatusUpdate(String channel, String status) {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["ch_t"] = channel.startsWith("LED") ? "LED" : "SHADE";
  doc["ch_addr"] = channel;
  doc["status"] = status;
  
  String payload;
  serializeJson(doc, payload);
  client.publish((String("MPS/global/UP/") + device_id + "/status").c_str(), payload.c_str());
  Serial.println("📤 Sent Status Update: " + payload);
}

void processLEDCommand(const JsonObject& command) {
  String ledAddr = command["ch_addr"].as<String>();

  // Check if it's the built-in LED
  if (ledAddr == "LED0") { // Assuming you want to control it via "LED0"
    if (command["cmd"] == 104) {
      String cmd_m = command["cmd_m"].as<String>();
      digitalWrite(BUILTIN_LED, cmd_m == "LED_ON" ? HIGH : LOW); // Set the built-in LED
      sendStatusUpdate(ledAddr, cmd_m == "LED_ON" ? "on" : "off");
      Serial.printf("💡 Built-in LED turned %s\n", cmd_m.c_str());
    } else if (command["cmd"] == 102) {
      // Brightness control (if applicable)
      int brightness = command["cmd_m"].as<int>();
      analogWrite(BUILTIN_LED, map(brightness, 0, 100, 0, 255));
      sendStatusUpdate(ledAddr, String(brightness) + "%");
      Serial.printf("💡 Built-in LED brightness set to %d%%\n", brightness);
    }
  } else {
    // Original LED control logic
    int ledIndex = ledAddr.substring(3).toInt() - 1;
    if (ledIndex >= 0 && ledIndex < sizeof(ledPins)/sizeof(ledPins[0])) {
      if (command["cmd"] == 104) {
        String cmd_m = command["cmd_m"].as<String>();
        digitalWrite(ledPins[ledIndex], cmd_m == "LED_ON" ? HIGH : LOW);
        sendStatusUpdate(ledAddr, cmd_m == "LED_ON" ? "on" : "off");
        Serial.printf("💡 %s turned %s\n", ledAddr.c_str(), cmd_m.c_str());
      } else if (command["cmd"] == 102) {
        int brightness = command["cmd_m"].as<int>();
        analogWrite(ledPins[ledIndex], map(brightness, 0, 100, 0, 255));
        sendStatusUpdate(ledAddr, String(brightness) + "%");
        Serial.printf("💡 %s brightness set to %d%%\n", ledAddr.c_str(), brightness);
      }
    }
  }
}




void processShadeCommand(const JsonObject& command) {
  String shadeAddr = command["ch_addr"].as<String>();
  int shadeIndex = shadeAddr.substring(5).toInt() - 1;

  if (shadeIndex >= 0 && shadeIndex < sizeof(shadePins)/sizeof(shadePins[0])) {
    if (command["cmd"] == 113) {
      digitalWrite(shadePins[shadeIndex], HIGH);
      sendStatusUpdate(shadeAddr, "open");
      Serial.printf("🛑 %s opened\n", shadeAddr.c_str());
    }
    else if (command["cmd"] == 114) {
      digitalWrite(shadePins[shadeIndex], LOW);
      sendStatusUpdate(shadeAddr, "closed");
      Serial.printf("🛑 %s closed\n", shadeAddr.c_str());
    }
    else if (command["cmd"] == 111) {
      // Assuming 'stop' means no signal
      digitalWrite(shadePins[shadeIndex], LOW); // Or whatever stops the motor
      sendStatusUpdate(shadeAddr, "stopped");
      Serial.printf("🛑 %s stopped\n", shadeAddr.c_str());
    }
  }
}


void sendPing() {
  JsonDocument pingDoc;
  pingDoc["device_id"] = device_id;
  pingDoc["status"] = "online";
  pingDoc["uptime"] = millis() / 1000;
  pingDoc["rssi"] = WiFi.RSSI();

  String pingBuffer;
  serializeJson(pingDoc, pingBuffer);
  client.publish("MPS/global/sessionPing", pingBuffer.c_str());
  Serial.println("📡 Sent Ping: " + pingBuffer);
}

void processSceneCommand(const JsonObject& command) {
  JsonVariant cmd_m = command["cmd_m"];
  JsonArray channels = command["ch_addr"].as<JsonArray>();

  // Handle LED ON/OFF commands
  if (cmd_m.is<const char*>()) { // String command
    String action = cmd_m.as<String>();
    if (action == "LED_ON" || action == "LED_OFF") {
      for (JsonVariant ch : channels) {
        int channelNum = ch.as<int>(); // Get integer from array
        if (channelNum >= 1 && channelNum <= 12) {
          String ledAddr = "LED" + String(channelNum);
          int ledIndex = channelNum - 1;
          digitalWrite(ledPins[ledIndex], action == "LED_ON" ? HIGH : LOW);
          sendStatusUpdate(ledAddr, action == "LED_ON" ? "on" : "off");
        }
      }
    }
  }
  // Handle brightness commands
  else if (cmd_m.is<JsonObject>()) { // Object command
    JsonObject brightnessCmd = cmd_m.as<JsonObject>();
    if (brightnessCmd["LED_BRIGHTNESS"].is<int>()) {
      int brightness = brightnessCmd["LED_BRIGHTNESS"];
      for (JsonVariant ch : channels) {
        int channelNum = ch.as<int>();
        if (channelNum >= 1 && channelNum <= 12) {
          String ledAddr = "LED" + String(channelNum);
          int ledIndex = channelNum - 1;
          analogWrite(ledPins[ledIndex], map(brightness, 0, 100, 0, 255));
          sendStatusUpdate(ledAddr, String(brightness) + "%");
        }
      }
    }
  }
}



void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("📩 Message received on topic: ");
  Serial.println(topic);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("❌ JSON Parsing Error: ");
    Serial.println(error.c_str());
    return;
  }

  String topicStr(topic);
  
  if (topicStr.endsWith("/config")) {
    Serial.println("⚙️ Received config message");
    

    // Add this line to send config response back
    sendConfigResponse();
  }
  else if (topicStr.endsWith("/control")) {
    Serial.println("🎛️ Received control command");
    JsonObject obj = doc.as<JsonObject>();
    String ch_type = obj["ch_t"].as<String>();
    if (ch_type == "LED") processLEDCommand(obj);
    else if (ch_type == "SHADE") processShadeCommand(obj);
  }
  else if (topicStr.endsWith("/scene")) { // Changed to else-if
    Serial.println("🎨 Received scene command");
    JsonObject obj = doc.as<JsonObject>();
    processSceneCommand(obj);
  }
}

void setup() {
  Serial.begin(9600);
  
  for (int pin : ledPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  for (int pin : shadePins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  
  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  if (millis() - lastPingTime >= pingInterval) {
    sendPing();
    lastPingTime = millis();
  }
}