#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "password.h"// Include your password.h file for WiFi credentials

// MQTT Broker Details
const char* mqtt_server = "192.168.29.98"; // Update with your broker IP
const int mqtt_port = 1883;
const char* mqtt_user = "mps-bam100";       // MQTT Username
const char* mqtt_password = "bam100";       // MQTT Password

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);

// Device Details
String device_id;

// Timer for Ping Interval
unsigned long lastPingTime = 0;
const unsigned long pingInterval = 30000; // 30 seconds

// LED & Shade Control Pins
const int ledPins[] = {2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32}; // GPIO pins for LEDs
const int shadePins[] = {33, 34, 35, 36}; // GPIO pins for Shades

// Function to Send Device Discovery Message
void sendDeviceDiscovery() {
  JsonDocument doc; // Use JsonDocument instead of StaticJsonDocument
  doc["device_id"] = device_id;
  doc["SNO"] = "234AM87695";
  doc["Firmware"] = "2.01";
  doc["MacAddr"] = WiFi.macAddress();

  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("MPS/global/discovery", buffer, true);
  Serial.println("📢 Published Discovery Data:");
  Serial.println(buffer);
}

// Function to Connect to WiFi
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
  
  // Set device ID based on your requirement
  device_id = "123456"; 
}

// Function to Connect to MQTT Broker
void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("🔄 Connecting to MQTT broker...");
    if (client.connect(device_id.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("✅ MQTT connected");
      sendDeviceDiscovery();
      client.subscribe(("MPS/device/" + device_id + "/control").c_str());
      Serial.println("📡 Subscribed to Control Commands");
    } else {
      Serial.print("❌ MQTT Connection Failed. State: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// Function to Process LED Commands
void processLEDCommand(const JsonObject& command) {
  String ledAddr = command["ch_addr"].as<String>();
  int ledIndex = ledAddr.substring(3).toInt() - 1; // Extract LED number (e.g., LED1 -> index=0)

  if (ledIndex >= 0 && ledIndex < sizeof(ledPins) / sizeof(ledPins[0])) {
    if (command["cmd"] == 104) { // Turn LED ON/OFF
      String cmd_m = command["cmd_m"].as<String>();
      digitalWrite(ledPins[ledIndex], cmd_m == "LED_ON" ? HIGH : LOW);
      Serial.printf("💡 %s turned %s\n", ledAddr.c_str(), cmd_m.c_str());
    } else if (command["cmd"] == 102) { // Adjust brightness
      int brightness = command["cmd_m"].as<int>();
      analogWrite(ledPins[ledIndex], map(brightness, 0, 100, 0, 255));
      Serial.printf("💡 %s brightness set to %d%%\n", ledAddr.c_str(), brightness);
    } else if (command["cmd"] == 105) { // Configure min/max brightness
      JsonArray cmd_m = command["cmd_m"].as<JsonArray>();
      int minBrightness = cmd_m[0];
      int maxBrightness = cmd_m[1];
      Serial.printf("💡 %s configured with min: %d%%, max: %d%%\n", ledAddr.c_str(), minBrightness, maxBrightness);
    }
  } else {
    Serial.printf("❌ Invalid LED address: %s\n", ledAddr.c_str());
  }
}

// Function to Process Shade Commands
void processShadeCommand(const JsonObject& command) {
  String shadeAddr = command["ch_addr"].as<String>();
  int shadeIndex = shadeAddr.substring(5).toInt() - 1; // Extract Shade number (e.g., SHADE1 -> index=0)

  if (shadeIndex >= 0 && shadeIndex < sizeof(shadePins) / sizeof(shadePins[0])) {
    String cmd_m = command["cmd_m"].as<String>();
    if (command["cmd"] == 113 && cmd_m == "shade_open") {
      digitalWrite(shadePins[shadeIndex], HIGH); // Open shade
      Serial.printf("🛑 %s opened\n", shadeAddr.c_str());
    } else if (command["cmd"] == 114 && cmd_m == "shade_close") {
      digitalWrite(shadePins[shadeIndex], LOW); // Close shade
      Serial.printf("🛑 %s closed\n", shadeAddr.c_str());
    } else if (command["cmd"] == 111 && cmd_m == "shade_stop") {
      digitalWrite(shadePins[shadeIndex], LOW); // Stop shade movement
      Serial.printf("🛑 %s stopped\n", shadeAddr.c_str());
    }
  } else {
    Serial.printf("❌ Invalid Shade address: %s\n", shadeAddr.c_str());
  }
}
// Function to Send Ping Message
void sendPing() {
  JsonDocument pingDoc; // Use JsonDocument instead of StaticJsonDocument or DynamicJsonDocument

  pingDoc["device_id"] = device_id;
  pingDoc["status"] = "online"; // You can add more fields if needed

  String pingBuffer;
  serializeJson(pingDoc, pingBuffer); // Serialize to a String

  client.publish("MPS/global/sessionPing", pingBuffer.c_str()); // Publish ping message
  Serial.println("📡 Sent Ping:");
  Serial.println(pingBuffer);
  delay(10);  // Add a small delay
}


// MQTT Callback Function to Handle Commands
void callback(char* topic, byte* payload, unsigned int length) {
   Serial.print("📩 Message received on topic: ");
   Serial.println(topic);

   String message;
   for (int i = 0; i < length; i++) {
     message += (char)payload[i];
   }
   Serial.println("📜 Message: " + message);

   JsonDocument doc; // Change this line as well.
   DeserializationError error = deserializeJson(doc, message);
   if (error) {
     Serial.print("❌ JSON Parsing Error: ");
     Serial.println(error.c_str());
     return;
   }

   // ✅ Handling Control Commands
   if (String(topic).endsWith("/control")) {
     Serial.println("🔹 Processing Control Command...");

     if (doc.is<JsonObject>()) {
       JsonObject obj = doc.as<JsonObject>();
       if (obj["ch_t"].is<String>()) { // Update here as well.
         String ch_t = obj["ch_t"].as<String>();
         if (ch_t == "LED") {
           processLEDCommand(obj);
         } else if (ch_t == "SHADE") {
           processShadeCommand(obj);
         }
       }
     }
   }
}

void setup() {
   Serial.begin(115200);
   for (int i = 0; i < sizeof(ledPins) / sizeof(ledPins[0]); i++) {
     pinMode(ledPins[i], OUTPUT);
   }
   for (int i = 0; i < sizeof(shadePins) / sizeof(shadePins[0]); i++) {
     pinMode(shadePins[i], OUTPUT);
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

   // Send Ping at Regular Intervals
   unsigned long currentTime = millis();
   if (currentTime - lastPingTime >= pingInterval) {
     sendPing();
     lastPingTime = currentTime;
   }
}
  /*
  The code is divided into several sections: 
  WiFi and MQTT Setup:  This section includes the WiFi and MQTT broker details, along with the MQTT client setup. Device Details:  This section defines the device ID and the GPIO pins for LEDs and shades. Device Discovery:  This function sends the device discovery message to the broker. WiFi and MQTT Connection:  These functions connect to the WiFi network and the MQTT broker. LED and Shade Control:  These functions process the LED and shade control commands received from the broker. MQTT Callback Function:  This function handles the incoming MQTT messages and calls the appropriate control functions. Setup and Loop Functions:  The setup function initializes the GPIO pins and connects to the WiFi and MQTT broker. The loop function checks the MQTT connection and processes incoming messages. 
  The code uses the  PubSubClient library to connect to the MQTT broker and handle incoming messages. It also uses the  ArduinoJson library to parse the JSON messages received from the broker. 
  Step 4: Upload the Code to ESP32 
  Now that we have the code ready, let’s upload it to the ESP32 board. Follow these steps to upload the code: 
  Connect the ESP32 board to your computer using a USB cable. Open the Arduino IDE and select the correct board and port from the Tools menu. Click the Upload button to compile and upload the code to the ESP32 board. 
  Once the code is uploaded successfully, open the Serial Monitor to view the debug messages. You should see the device connecting to the WiFi network and the MQTT broker. 
  Step 5: Test the MQTT Control Commands 
  To test the MQTT control commands, you can use an MQTT client like  MQTT Explorer or  MQTT.fx. Follow these steps to send control commands to the ESP32 device: 
  Open the MQTT client and connect to the MQTT broker. Subscribe to the topic “MPS/device/123456/control” (replace 123456 with your device ID). Send control commands to the device using the following JSON format: 
  {
    "device_id": "123456",
    "ch_t": "LED",
    "ch_addr": "LED1",
    "cmd": 104,
    "cmd_m": "LED_ON"
  }
  
  You can send different control commands to control the LEDs and shades connected to the ESP32 board. The device should respond to the commands and control the LEDs and shades accordingly. 
  Conclusion 
  In this tutorial, you learned how to control
  */