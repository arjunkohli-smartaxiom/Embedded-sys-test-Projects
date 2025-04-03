#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WebServer.h> // For activation portal

// MQTT Config
const char* MQTT_BROKER = "192.168.29.98";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "mps-bam100";
const char* MQTT_PASS = "bam100";

// Device Info
const String DEVICE_ID = "IoT_Device_" + String((uint32_t)ESP.getEfuseMac(), HEX);
const String CMD_TOPIC = DEVICE_ID + "/cmd";
const String STATUS_TOPIC = DEVICE_ID + "/status";

// Pins
#define LED_PIN 2

WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiManager wifiManager;
WebServer server(80);

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  
  // Start WiFi
  wifiManager.autoConnect("IoT_Device_AP");
  
  // Activation Portal
  server.on("/activate", HTTP_POST, [](){
    String email = server.arg("email");
    mqtt.publish("devices/register", ("{\"email\":\"" + email + "\",\"deviceId\":\"" + DEVICE_ID + "\"}").c_str());
    server.send(200, "text/plain", "Check email for activation code");
  });
  server.begin();

  // MQTT Setup
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
    String msg;
    for(int i=0; i<length; i++) msg += (char)payload[i];
    digitalWrite(LED_PIN, msg == "ON" ? HIGH : LOW);
  });
}

void loop() {
  server.handleClient();
  
  if (!mqtt.connected()) {
    if (mqtt.connect(DEVICE_ID.c_str(), MQTT_USER, MQTT_PASS)) {
      mqtt.subscribe(CMD_TOPIC.c_str());
      mqtt.publish(STATUS_TOPIC.c_str(), "OFF", true);
    }
  }
  mqtt.loop();
}