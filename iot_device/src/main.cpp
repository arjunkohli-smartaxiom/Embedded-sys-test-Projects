#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WebServer.h>

// MQTT Config
const char* MQTT_BROKER = "192.168.29.98";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "mps-bam100";
const char* MQTT_PASS = "bam100";

// Device Info
const String DEVICE_ID = "IoT_Device_" + String((uint32_t)ESP.getEfuseMac(), HEX);
const String CMD_TOPIC = DEVICE_ID + "/cmd";
const String STATUS_TOPIC = DEVICE_ID + "/status";

#define LED_PIN 2

WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiManager wifiManager;
WebServer server(80);

void reconnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect(DEVICE_ID.c_str(), MQTT_USER, MQTT_PASS)) {
      mqtt.subscribe(CMD_TOPIC.c_str());
      mqtt.publish(STATUS_TOPIC.c_str(), "OFF", true); // Retain state
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Initialize LED OFF
  
  wifiManager.autoConnect("IoT_Device_AP");
  
  server.on("/activate", HTTP_POST, [](){
    String email = server.arg("email");
    mqtt.publish("devices/register", ("{\"email\":\"" + email + "\",\"deviceId\":\"" + DEVICE_ID + "\"}").c_str());
    server.send(200, "text/plain", "Check email for activation code");
  });
  server.begin();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
    String msg;
    for(int i=0; i<length; i++) msg += (char)payload[i];
    msg.trim(); // Critical for "ON"/"OFF" comparison
    Serial.print("Received: ");
    Serial.println(msg);
    digitalWrite(LED_PIN, msg == "ON" ? HIGH : LOW);
    mqtt.publish(STATUS_TOPIC.c_str(), msg.c_str(), true); // Update status
  });
  Serial.print("Device ID: ");
Serial.println(DEVICE_ID);
Serial.print("Subscribed to: ");
Serial.println(CMD_TOPIC);

}

void loop() {
  server.handleClient();
  if (!mqtt.connected()) reconnect();
  mqtt.loop();
}
