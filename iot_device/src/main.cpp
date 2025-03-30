#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

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
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  
  startWiFi();
  
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
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