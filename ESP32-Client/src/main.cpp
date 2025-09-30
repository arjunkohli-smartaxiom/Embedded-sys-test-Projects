#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include "password.h"

// EEPROM Configuration
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASSWORD_ADDR 64
#define MQTT_SERVER_ADDR 128
#define CONFIG_FLAG_ADDR 192

// Default MQTT Broker Details
const char* default_mqtt_server = "35.200.133.222";  
const int mqtt_port = 1883;
const char* mqtt_user = "mps-bam100";
const char* mqtt_password = "bam100";

// Dynamic credentials
String wifi_ssid = "";
String wifi_password = "";
String mqtt_server = "";

// Configuration mode flag
bool configMode = false;
const unsigned long configTimeout = 30000;
unsigned long configStartTime = 0;

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);

// Device Details
String device_id = "123456";
const char* device_serial = "234AM87695";
const char* firmware_version = "2.01";

// PIR Sensor Configuration - Dynamic system
#define PIR_DE_PIN 4        // Keep original pin 4
#define PIR_RX_PIN 16       // Keep original pin 16
#define PIR_TX_PIN 17       // Keep original pin 17

// Dynamic PIR Sensor Configuration
struct PIRSensor {
  uint8_t sensor_id;
  uint8_t port;
  bool motion_detected;
  bool last_send_status;
  unsigned long last_motion_time;
  unsigned long timer_start;
  bool timer_active;
  bool first_motion_sent;  // Track if first motion MQTT was sent
};

// Default sensor configuration (can be changed via serial commands)
PIRSensor pirSensor = {2, 1, false, false, 0, 0, false, false}; // ID=2, Port=1, first_motion_sent=false

// Use ESP32 Hardware Serial (Serial2) with original pins
HardwareSerial RS485Serial(2); // RX=16, TX=17
ModbusMaster node;

// PIR Sensor Variables
bool motionDetected = false;
bool lastMotionState = false;
unsigned long lastMotionTime = 0;
const unsigned long motionDebounceTime = 1000; // 1 second debounce

// Dynamic timer configuration
unsigned long SENSE_TIMEOUT = 30 * 1000; // 30 seconds for testing (change to 15 * 60 * 1000 for 15 minutes)
unsigned long MOTION_CHECK_INTERVAL = 60 * 1000; // 1 minute interval to check motion after motion detected

// Timer for Ping Interval
unsigned long lastPingTime = 0;
const unsigned long pingInterval = 30000;

// LED & Shade Control Pins (from your main code)
const int ledPins[] = {2, 5, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32};
const int shadePins[] = {33, 34, 35, 36};

// EEPROM Functions
void initEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
}

void saveCredentials() {
  for (int i = 0; i < wifi_ssid.length(); i++) {
    EEPROM.write(SSID_ADDR + i, wifi_ssid[i]);
  }
  EEPROM.write(SSID_ADDR + wifi_ssid.length(), '\0');
  
  for (int i = 0; i < wifi_password.length(); i++) {
    EEPROM.write(PASSWORD_ADDR + i, wifi_password[i]);
  }
  EEPROM.write(PASSWORD_ADDR + wifi_password.length(), '\0');
  
  for (int i = 0; i < mqtt_server.length(); i++) {
    EEPROM.write(MQTT_SERVER_ADDR + i, mqtt_server[i]);
  }
  EEPROM.write(MQTT_SERVER_ADDR + mqtt_server.length(), '\0');
  
  EEPROM.write(CONFIG_FLAG_ADDR, 1);
  EEPROM.commit();
  Serial.println("✅ Credentials saved to EEPROM");
}

void loadCredentials() {
  Serial.println("🔍 Loading credentials from EEPROM...");
  
  if (EEPROM.read(CONFIG_FLAG_ADDR) != 1) {
    Serial.println("📝 No saved credentials found, using defaults");
    wifi_ssid = String(ssid);
    wifi_password = String(password);
    mqtt_server = String(default_mqtt_server);
    Serial.println("📋 Default credentials loaded:");
    Serial.println("   SSID: " + wifi_ssid);
    Serial.println("   MQTT Server: " + mqtt_server);
    return;
  }
  
  wifi_ssid = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(SSID_ADDR + i);
    if (c == '\0') break;
    wifi_ssid += c;
  }
  
  wifi_password = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(PASSWORD_ADDR + i);
    if (c == '\0') break;
    wifi_password += c;
  }
  
  mqtt_server = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(MQTT_SERVER_ADDR + i);
    if (c == '\0') break;
    mqtt_server += c;
  }
  
  Serial.println("✅ Loaded credentials from EEPROM:");
  Serial.println("   SSID: " + wifi_ssid);
  Serial.println("   Password: " + String(wifi_password.length()) + " characters");
  Serial.println("   MQTT Server: " + mqtt_server);
}

void clearCredentials() {
  for (int i = 0; i < 64; i++) {
    EEPROM.write(SSID_ADDR + i, 0);
    EEPROM.write(PASSWORD_ADDR + i, 0);
    EEPROM.write(MQTT_SERVER_ADDR + i, 0);
  }
  EEPROM.write(CONFIG_FLAG_ADDR, 0);
  EEPROM.commit();
  Serial.println("🗑️ Credentials cleared from EEPROM");
}

// Forward declaration
void handleConfigInput(String input);
void showCurrentConfig();
void preTransmission();
void postTransmission();
void sendPIRStatusUpdate(String status);
void sendSenseTimeoutMessage();
void handlePIRConfig(String input);
void showPIRConfig();
uint8_t autoDetectPIRID();

// Serial Configuration Functions
void showConfigMenu() {
  Serial.println("\n=== ESP32 CONFIG ===");
  Serial.println("SSID: " + wifi_ssid);
  Serial.println("MQTT: " + mqtt_server);
  Serial.println("\nCommands:");
  Serial.println("'config' - Change settings");
  Serial.println("'show' - Display current configuration");
  Serial.println("'skip' - Use current settings");
  Serial.println("'clear' - Reset to defaults");
  Serial.println("\n⏰ Timeout: 30 seconds");
}

void showCurrentConfig() {
  Serial.println("\n📋 === CURRENT CONFIGURATION ===");
  Serial.println("🔧 Device Information:");
  Serial.println("   Device ID: " + device_id);
  Serial.println("   Serial Number: " + String(device_serial));
  Serial.println("   Firmware Version: " + String(firmware_version));
  Serial.println("   MAC Address: " + WiFi.macAddress());
  
  Serial.println("\n🌐 Network Configuration:");
  Serial.println("   WiFi SSID: " + wifi_ssid);
  Serial.println("   WiFi Password: " + String(wifi_password.length()) + " characters");
  Serial.println("   WiFi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("   IP Address: " + WiFi.localIP().toString());
    Serial.println("   Gateway: " + WiFi.gatewayIP().toString());
    Serial.println("   Subnet: " + WiFi.subnetMask().toString());
    Serial.println("   DNS: " + WiFi.dnsIP().toString());
    Serial.println("   RSSI: " + String(WiFi.RSSI()) + " dBm");
  }
  
  Serial.println("\n📡 MQTT Configuration:");
  Serial.println("   MQTT Server: " + mqtt_server);
  Serial.println("   MQTT Port: " + String(mqtt_port));
  Serial.println("   MQTT User: " + String(mqtt_user));
  Serial.println("   MQTT Status: " + String(client.connected() ? "Connected" : "Disconnected"));
  
  if (client.connected()) {
    Serial.println("   Subscribed Topics:");
    Serial.println("     - MPS/global/" + device_id + "/config");
    Serial.println("     - MPS/global/" + device_id + "/control");
    Serial.println("     - MPS/global/" + device_id + "/reboot");
    Serial.println("     - MPS/global/" + device_id + "/scene");
  }
  
  showPIRConfig();
  
  Serial.println("\n💾 EEPROM Status:");
  Serial.println("   Config Saved: " + String(EEPROM.read(CONFIG_FLAG_ADDR) == 1 ? "Yes" : "No"));
  Serial.println("   Uptime: " + String(millis() / 1000) + " seconds");
  Serial.println("================================\n");
}

void showPIRConfig() {
  Serial.println("\n🔍 PIR Sensor Configuration:");
  Serial.println("   Sensor ID: " + String(pirSensor.sensor_id));
  Serial.println("   Port: " + String(pirSensor.port));
  Serial.println("   Port Address: Port-" + String(pirSensor.port) + "_" + String(pirSensor.sensor_id));
  Serial.println("   Motion Status: " + String(pirSensor.motion_detected ? "DETECTED" : "NO MOTION"));
  Serial.println("   Timer Active: " + String(pirSensor.timer_active ? "YES" : "NO"));
  Serial.println("   First Motion Sent: " + String(pirSensor.first_motion_sent ? "YES" : "NO"));
  Serial.println("   Timeout: " + String(SENSE_TIMEOUT / 1000) + " seconds");
  Serial.println("   Motion Check Interval: " + String(MOTION_CHECK_INTERVAL / 1000) + " seconds");
  Serial.println("   DE Pin: " + String(PIR_DE_PIN));
  Serial.println("   RX Pin: " + String(PIR_RX_PIN));
  Serial.println("   TX Pin: " + String(PIR_TX_PIN));
  Serial.println("\n📝 PIR Commands:");
  Serial.println("   'pirconfig' - Configure PIR sensor");
  Serial.println("   'pirstatus' - Show PIR status");
  Serial.println("   'pirid X' - Set sensor ID to X");
  Serial.println("   'pirport X' - Set port to X");
  Serial.println("   'pirtimer X' - Set timeout to X seconds");
  Serial.println("   'pirinterval X' - Set motion check interval to X seconds");
  Serial.println("   'pirscan' - Auto-detect PIR sensor ID");
}

void handleSerialInput() {
    if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    String command = input;
        command.toLowerCase();
    
    if (command == "config") {
      configMode = true;
      configStartTime = millis();
      Serial.println("\n🔧 Config Mode:");
      Serial.println("📡 MQTT Server IP (current: " + mqtt_server + "):");
      Serial.println("   Press Enter to keep current value");
    }
    else if (command == "show") {
      showCurrentConfig();
    }
    else if (command == "skip") {
      Serial.println("\nUsing current settings...");
      configMode = false;
    }
    else if (command == "clear") {
      clearCredentials();
      loadCredentials();
      showConfigMenu();
    }
    else if (command == "pirconfig") {
      showPIRConfig();
    }
    else if (command == "pirstatus") {
      showPIRConfig();
    }
    else if (command.startsWith("pirid ")) {
      int newId = command.substring(6).toInt();
        if (newId >= 1 && newId <= 255) {
        pirSensor.sensor_id = newId;
        Serial.println("✅ PIR Sensor ID set to: " + String(newId));
        Serial.println("   Port Address: Port-" + String(pirSensor.port) + "_" + String(pirSensor.sensor_id));
        } else {
            Serial.println("❌ Invalid ID! Use 1-255");
        }
    }
    else if (command.startsWith("pirport ")) {
      int newPort = command.substring(8).toInt();
      if (newPort >= 1 && newPort <= 10) {
        pirSensor.port = newPort;
        Serial.println("✅ PIR Port set to: " + String(newPort));
        Serial.println("   Port Address: Port-" + String(pirSensor.port) + "_" + String(pirSensor.sensor_id));
      } else {
        Serial.println("❌ Invalid Port! Use 1-10");
      }
    }
    else if (command.startsWith("pirtimer ")) {
      int newTimer = command.substring(9).toInt();
      if (newTimer >= 5 && newTimer <= 3600) {
        SENSE_TIMEOUT = newTimer * 1000;
        Serial.println("✅ PIR Timeout set to: " + String(newTimer) + " seconds");
      } else {
        Serial.println("❌ Invalid Timer! Use 5-3600 seconds");
      }
    }
    else if (command.startsWith("pirinterval ")) {
      int newInterval = command.substring(12).toInt();
      if (newInterval >= 1 && newInterval <= 300) {
        MOTION_CHECK_INTERVAL = newInterval * 1000;
        Serial.println("✅ PIR Motion Check Interval set to: " + String(newInterval) + " seconds");
      } else {
        Serial.println("❌ Invalid Interval! Use 1-300 seconds");
      }
    }
    else if (command == "pirscan") {
      Serial.println("🔍 Scanning for PIR sensors...");
      uint8_t detectedID = autoDetectPIRID();
      pirSensor.sensor_id = detectedID;
      node.begin(pirSensor.sensor_id, RS485Serial);
      Serial.println("✅ PIR sensor updated with detected ID: " + String(detectedID));
    }
    else if (configMode) {
      handleConfigInput(input);
    }
    else {
      Serial.println("\nUnknown command. Try: config, show, skip, clear, pirconfig, pirstatus");
      Serial.println("PIR Commands: pirid X, pirport X, pirtimer X, pirinterval X, pirscan");
    }
  }
}

void handleConfigInput(String input) {
  static int configStep = 0;
  
  if (configStep == 0) {
    if (input.length() > 0) {
      mqtt_server = input;
      Serial.println("✅ MQTT Server: " + mqtt_server);
    } else {
      Serial.println("⏭️ Keeping current MQTT Server: " + mqtt_server);
    }
    Serial.println("\n📶 WiFi SSID (current: " + wifi_ssid + "):");
    Serial.println("   Press Enter to keep current SSID");
    configStep = 1;
  }
  else if (configStep == 1) {
    if (input.length() > 0) {
      wifi_ssid = input;
      Serial.println("✅ SSID: " + wifi_ssid);
        } else {
      Serial.println("⏭️ Keeping current SSID: " + wifi_ssid);
    }
    Serial.println("\n🔐 WiFi Password (current: " + String(wifi_password.length()) + " characters):");
    Serial.println("   Press Enter to keep current password");
    configStep = 2;
  }
  else if (configStep == 2) {
    if (input.length() > 0) {
      wifi_password = input;
      Serial.println("✅ Password: Updated");
    } else {
      Serial.println("⏭️ Keeping current password");
    }
    
    Serial.println("\n💾 Saving configuration...");
    saveCredentials();
    
    Serial.println("✅ Configuration saved!");
    Serial.println("📋 Final Settings:");
    Serial.println("   MQTT Server: " + mqtt_server);
    Serial.println("   WiFi SSID: " + wifi_ssid);
    Serial.println("   WiFi Password: " + String(wifi_password.length()) + " characters");
    Serial.println("\n🔄 Restarting ESP32...");
    
    configMode = false;
    configStep = 0;
    delay(2000);
    esp_restart();
  }
}

void checkConfigTimeout() {
  if (configMode && (millis() - configStartTime > configTimeout)) {
    Serial.println("\nTimeout! Using current settings...");
    configMode = false;
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: " + wifi_ssid + "...");
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    } else {
    Serial.println("\n❌ WiFi connection failed!");
    Serial.println("Please check your credentials and try again.");
  }
}

// Function to handle the reboot command
void handleRebootCommand(const JsonDocument& doc) {
  if (doc["deviceId"].is<String>() && doc["deviceId"] == "reboot") {
    Serial.println("🔄 Rebooting ESP32...");
    delay(1000);
    esp_restart();
  }
}

void sendDeviceDiscovery() {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["SNO"] = "234AM87697";
  doc["Firmware"] = "v1.0.0.1";
  doc["MacAddr"] = WiFi.macAddress();

  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("MPS/global/discovery", buffer, true);
  Serial.println("📢 Published Discovery Data:");
  Serial.println(buffer);
}

void sendConfigResponse() {
  JsonDocument doc;
  doc["ch_t"] = "LED";
  doc["ch_addr"] = "LED1";
  doc["cmd"] = 100;
  doc["cmd_m"] = "config";

  char buffer[256];
  serializeJson(doc, buffer);
  String topic = String("MPS/global/") + device_id + "/config";
  client.publish(topic.c_str(), buffer);
  Serial.println("📤 Sent Config Response:");
  Serial.println(buffer);
}

void connectToMQTT() {
  int attempts = 0;
  const int maxAttempts = 5;
  
  while (!client.connected() && attempts < maxAttempts) {
    attempts++;
    Serial.println("🔄 Connecting to MQTT broker: " + mqtt_server + " (Attempt " + String(attempts) + "/" + String(maxAttempts) + ")");
    Serial.println("📋 Device ID: " + device_id);
    Serial.println("📋 MQTT User: " + String(mqtt_user));
    
    if (client.connect(device_id.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("✅ MQTT connected successfully!");
      Serial.println("📡 Device ID: " + device_id);
            
      String configTopic = "MPS/global/" + device_id + "/config";
      String controlTopic = "MPS/global/" + device_id + "/control";
      String rebootTopic = "MPS/global/" + device_id + "/reboot";
      String sceneTopic = "MPS/global/" + device_id + "/scene";
      
      client.subscribe(configTopic.c_str());
      client.subscribe(controlTopic.c_str());
      client.subscribe(rebootTopic.c_str());
      client.subscribe(sceneTopic.c_str());
      
      Serial.println("📡 Subscribed to topics:");
      Serial.println("   - " + configTopic);
      Serial.println("   - " + controlTopic);
      Serial.println("   - " + rebootTopic);
      Serial.println("   - " + sceneTopic);
      
      sendDeviceDiscovery();
        return;
    } else {
      Serial.print("❌ MQTT Connection Failed. State: ");
      Serial.println(client.state());
      Serial.println("⏳ Retrying in 5 seconds...");
      delay(5000);
    }
  }
  
  if (attempts >= maxAttempts) {
    Serial.println("❌ MQTT connection failed after " + String(maxAttempts) + " attempts!");
    Serial.println("🔄 Will retry in main loop...");
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

// PIR Sensor Functions
void initPIRSensor() {
  Serial.println("🔍 Initializing PIR Sensor...");
  
  pinMode(PIR_DE_PIN, OUTPUT);
  digitalWrite(PIR_DE_PIN, LOW);
  
  RS485Serial.begin(9600, SERIAL_8N1, PIR_RX_PIN, PIR_TX_PIN);
  
  // Auto-detect PIR sensor ID
  pirSensor.sensor_id = autoDetectPIRID();
  
  Serial.println("   PIR ID: " + String(pirSensor.sensor_id));
  Serial.println("   Port: " + String(pirSensor.port));
  Serial.println("   Port Address: Port-" + String(pirSensor.port) + "_" + String(pirSensor.sensor_id));
  Serial.println("   DE Pin: " + String(PIR_DE_PIN));
  Serial.println("   RX Pin: " + String(PIR_RX_PIN));
  Serial.println("   TX Pin: " + String(PIR_TX_PIN));
  
  node.begin(pirSensor.sensor_id, RS485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
    
  Serial.println("✅ PIR Sensor initialized");
  
  // Send initial PIR status
  delay(1000);
  sendPIRStatusUpdate("no_motion");
}

void sendPIRStatusUpdate(String status) {
  if (client.connected()) {
    JsonDocument doc;
    doc["ch_t"] = "PIR";
    
    // Dynamic port address: "Port-{port}_{sensor_id}"
    String port_addr = "Port-" + String(pirSensor.port) + "_" + String(pirSensor.sensor_id);
    doc["ch_addr"] = port_addr;
    doc["cmd"] = 115;             // Match the command code from working device
    
    // Dynamic status message
    String cmd_m = "PIR State = " + String(status == "motion_detected" ? "1" : "0");
    doc["cmd_m"] = cmd_m;
    
    String payload;
    serializeJson(doc, payload);
    client.publish((String("MPS/global/UP/") + device_id + "/status").c_str(), payload.c_str());
    Serial.println("📤 Sent PIR Status: " + payload);
  }
}

void sendSenseTimeoutMessage() {
  if (client.connected()) {
    JsonDocument doc;
    doc["ch_t"] = "PIR";
    
    // Dynamic port address
    String port_addr = "Port-" + String(pirSensor.port) + "_" + String(pirSensor.sensor_id);
    doc["ch_addr"] = port_addr;
    doc["cmd"] = 115;
    doc["cmd_m"] = "SENSE_TIMEOUT";
    
    String payload;
    serializeJson(doc, payload);
    client.publish((String("MPS/global/UP/") + device_id + "/status").c_str(), payload.c_str());
    Serial.println("📤 Sent Sense Timeout: " + payload);
  }
}

// Timer callback function removed - we now only send MQTT when actual motion stops

void checkPIRMotion() {
    uint8_t result = node.readHoldingRegisters(0x0006, 1);
    
    if (result == node.ku8MBSuccess) {
        uint16_t status = node.getResponseBuffer(0);
        bool newMotionState = (status == 0x0001);
        
        // Always show motion status on serial for debugging
        if (newMotionState != lastMotionState && (millis() - lastMotionTime > motionDebounceTime)) {
            motionDetected = newMotionState;
            lastMotionState = newMotionState;
            lastMotionTime = millis();
            
            if (motionDetected) {
                Serial.println("🔴 PIR MOTION DETECTED!");
                pirSensor.motion_detected = 1;
                pirSensor.last_motion_time = millis();
                
                // Check if this is first motion or during timer period
                if (!pirSensor.first_motion_sent) {
                    // FIRST MOTION: Send MQTT immediately to turn ON lights
                    Serial.println("📤 FIRST MOTION - Sending MQTT config to turn ON lights");
                    sendPIRStatusUpdate("motion_detected");
                    pirSensor.first_motion_sent = true;
                    pirSensor.timer_start = millis();
                    pirSensor.timer_active = true;
                    Serial.println("⏰ Timer started - subsequent motion will show on serial only");
                } else {
                    // SUBSEQUENT MOTION: Only show on serial, NO MQTT
                    Serial.println("⏳ Motion during timer period - Serial only (NO MQTT)");
                }
            } else {
                Serial.println("🟢 PIR NO MOTION");
                
                // Only send MQTT if timer is not active (timer expired or no motion before timer started)
                if (!pirSensor.timer_active) {
                    Serial.println("📤 Sending no motion MQTT to turn OFF lights");
                    sendPIRStatusUpdate("no_motion");
                    // Reset for next cycle only when timer is not active
                    pirSensor.first_motion_sent = false;
                } else {
                    Serial.println("⏳ No motion detected but timer still active - showing on serial only");
                    // Don't reset flags when timer is still active
                }
            }
        }
        
        // Check timer timeout - send no motion MQTT when timer expires
        if (pirSensor.timer_active && (millis() - pirSensor.timer_start > SENSE_TIMEOUT)) {
            Serial.println("⏰ Timer expired - sending no motion MQTT to turn OFF lights");
            Serial.println("   Timer was active for: " + String((millis() - pirSensor.timer_start) / 1000) + " seconds");
            sendPIRStatusUpdate("no_motion");
            pirSensor.motion_detected = 0;
            pirSensor.first_motion_sent = false;
            pirSensor.timer_active = false;
        }
        
        // Debug timer status
        if (pirSensor.timer_active) {
            unsigned long elapsed = (millis() - pirSensor.timer_start) / 1000;
            unsigned long remaining = (SENSE_TIMEOUT / 1000) - elapsed;
            if (elapsed % 5 == 0) { // Print every 5 seconds
                Serial.println("⏱️ Timer status: " + String(elapsed) + "s elapsed, " + String(remaining) + "s remaining");
            }
        }
        
    } else {
        Serial.print("❌ PIR Communication Error: ");
        Serial.println(result);
    }
}

void preTransmission() {
  digitalWrite(PIR_DE_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(PIR_DE_PIN, LOW);
}

// Auto-detect PIR sensor ID by scanning common IDs
uint8_t autoDetectPIRID() {
  Serial.println("🔍 Auto-detecting PIR sensor ID...");
  
  // Common PIR sensor IDs to try
  uint8_t commonIDs[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  uint8_t numIDs = sizeof(commonIDs) / sizeof(commonIDs[0]);
  
  for (int i = 0; i < numIDs; i++) {
    uint8_t testID = commonIDs[i];
    Serial.print("   Testing ID " + String(testID) + "... ");
    
    // Create temporary node for testing
    ModbusMaster testNode;
    testNode.begin(testID, RS485Serial);
    testNode.preTransmission(preTransmission);
    testNode.postTransmission(postTransmission);
    
    // Try to read a register
    uint8_t result = testNode.readHoldingRegisters(0x0006, 1);
    
    if (result == testNode.ku8MBSuccess) {
      Serial.println("✅ Found!");
      Serial.println("🎯 Auto-detected PIR Sensor ID: " + String(testID));
      return testID;
    } else {
      Serial.println("❌ No response");
    }
    
    delay(100); // Small delay between tests
  }
  
  Serial.println("⚠️ No PIR sensor detected, using default ID: 2");
  return 2; // Default fallback
}

void processLEDCommand(const JsonObject& command) {
  String ledAddr = command["ch_addr"].as<String>();
  int cmd = command["cmd"];
  String cmd_m = command["cmd_m"].as<String>();
  
  Serial.println("🔍 LED Command Details:");
  Serial.println("   Address: " + ledAddr);
  Serial.println("   Command: " + String(cmd));
  Serial.println("   Action: " + cmd_m);

  if (ledAddr == "LED0") {
    if (cmd == 104) {
      digitalWrite(BUILTIN_LED, cmd_m == "LED_ON" ? HIGH : LOW);
      sendStatusUpdate(ledAddr, cmd_m == "LED_ON" ? "on" : "off");
      Serial.println("💡 LED0: " + String(cmd_m == "LED_ON" ? "ON" : "OFF"));
    } else if (cmd == 102) {
      int brightness = command["cmd_m"].as<int>();
      analogWrite(BUILTIN_LED, map(brightness, 0, 100, 0, 255));
      sendStatusUpdate(ledAddr, String(brightness) + "%");
      Serial.println("💡 LED0: Brightness " + String(brightness) + "%");
    }
  } else {
    int ledIndex = ledAddr.substring(3).toInt() - 1;
    Serial.println("🔢 LED Index: " + String(ledIndex));
    
    if (ledIndex >= 0 && ledIndex < sizeof(ledPins)/sizeof(ledPins[0])) {
      if (cmd == 104) {
        digitalWrite(ledPins[ledIndex], cmd_m == "LED_ON" ? HIGH : LOW);
        sendStatusUpdate(ledAddr, cmd_m == "LED_ON" ? "on" : "off");
        Serial.println("💡 " + ledAddr + ": " + String(cmd_m == "LED_ON" ? "ON" : "OFF"));
      } else if (cmd == 102) {
        int brightness = command["cmd_m"].as<int>();
        analogWrite(ledPins[ledIndex], map(brightness, 0, 100, 0, 255));
        sendStatusUpdate(ledAddr, String(brightness) + "%");
        Serial.println("💡 " + ledAddr + ": Brightness " + String(brightness) + "%");
      }
    } else {
      Serial.println("❌ Invalid LED index: " + String(ledIndex));
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
      Serial.println(shadeAddr + ": OPENED");
    }
    else if (command["cmd"] == 114) {
      digitalWrite(shadePins[shadeIndex], LOW);
      sendStatusUpdate(shadeAddr, "closed");
      Serial.println(shadeAddr + ": CLOSED");
    }
    else if (command["cmd"] == 111) {
      digitalWrite(shadePins[shadeIndex], LOW);
      sendStatusUpdate(shadeAddr, "stopped");
      Serial.println(shadeAddr + ": STOPPED");
    }
  }
}

void sendPing() {
  JsonDocument pingDoc;
  pingDoc["device_id"] = device_id;
  pingDoc["status"] = "online";
  pingDoc["uptime"] = millis() / 1000;
  pingDoc["rssi"] = WiFi.RSSI();
  pingDoc["pir_motion"] = motionDetected;

  String pingBuffer;
  serializeJson(pingDoc, pingBuffer);
  client.publish("MPS/global/sessionPing", pingBuffer.c_str());
  Serial.println("📡 Sent Ping: " + pingBuffer);
}

void processSceneCommand(const JsonObject& command) {
  Serial.println("🎨 Processing Scene Command...");
  JsonVariant cmd_m = command["cmd_m"];
  JsonArray channels = command["ch_addr"].as<JsonArray>();

  Serial.println("📋 Scene Command Details:");
  Serial.println("   Channels: " + String(channels.size()));

  if (cmd_m.is<const char*>()) {
    String action = cmd_m.as<String>();
    Serial.println("   Action: " + action);
    if (action == "LED_ON" || action == "LED_OFF") {
      for (JsonVariant ch : channels) {
        int channelNum = ch.as<int>();
        if (channelNum >= 1 && channelNum <= 12) {
          String ledAddr = "LED" + String(channelNum);
          int ledIndex = channelNum - 1;
          digitalWrite(ledPins[ledIndex], action == "LED_ON" ? HIGH : LOW);
          sendStatusUpdate(ledAddr, action == "LED_ON" ? "on" : "off");
          Serial.println("💡 " + ledAddr + ": " + String(action == "LED_ON" ? "ON" : "OFF"));
        }
      }
    }
  }
  else if (cmd_m.is<JsonObject>()) {
    JsonObject brightnessCmd = cmd_m.as<JsonObject>();
    if (brightnessCmd["LED_BRIGHTNESS"].is<int>()) {
      int brightness = brightnessCmd["LED_BRIGHTNESS"];
      Serial.println("   Brightness: " + String(brightness) + "%");
      for (JsonVariant ch : channels) {
        int channelNum = ch.as<int>();
        if (channelNum >= 1 && channelNum <= 12) {
          String ledAddr = "LED" + String(channelNum);
          int ledIndex = channelNum - 1;
          analogWrite(ledPins[ledIndex], map(brightness, 0, 100, 0, 255));
          sendStatusUpdate(ledAddr, String(brightness) + "%");
          Serial.println("💡 " + ledAddr + ": Brightness " + String(brightness) + "%");
        }
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("📩 Message received on topic: ");
  Serial.println(topic);
  Serial.print("📦 Payload length: ");
  Serial.println(length);
  Serial.print("📄 Raw payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

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
    JsonObject obj = doc.as<JsonObject>();
    if (obj["cmd"].is<int>() && obj["cmd"] == 106 && obj["cmd_m"] == "config") {
      sendConfigResponse();
    }
  }
  else if (topicStr.endsWith("/control")) {
    Serial.println("🎮 Control command received");
    JsonObject obj = doc.as<JsonObject>();
    String ch_type = obj["ch_t"].as<String>();
    String ch_addr = obj["ch_addr"].as<String>();
    int cmd = obj["cmd"];
    Serial.println("📋 Type: " + ch_type + ", Address: " + ch_addr + ", Cmd: " + String(cmd));
    
    if (ch_type == "LED") {
      Serial.println("💡 Processing LED command...");
      processLEDCommand(obj);
    }
    else if (ch_type == "SHADE") {
      Serial.println("🪟 Processing Shade command...");
      processShadeCommand(obj);
    }
  }
  else if (topicStr.endsWith("/scene")) {
    Serial.println("🎨 Received scene command");
    JsonObject obj = doc.as<JsonObject>();
    processSceneCommand(obj);
  }
  else if (topicStr.endsWith("/reboot")) {
    Serial.println("🔄 Received reboot command");
    handleRebootCommand(doc);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n🚀 ESP32 Starting...");
  Serial.println("📋 Device ID: " + device_id);
  Serial.println("📋 Serial Number: " + String(device_serial));
  Serial.println("📋 Firmware Version: " + String(firmware_version));
  
  initEEPROM();
  loadCredentials();
  
  showConfigMenu();
  configStartTime = millis();
  
  while (configMode || (millis() - configStartTime < configTimeout)) {
    handleSerialInput();
    checkConfigTimeout();
    delay(10);
  }
  
  if (!configMode) {
    Serial.println("\n🎯 Starting normal operation...");
    Serial.println("📡 Final Configuration:");
    Serial.println("   WiFi SSID: " + wifi_ssid);
    Serial.println("   MQTT Server: " + mqtt_server);
    Serial.println("   Device ID: " + device_id);
    
    configMode = false;
    
    Serial.println("🔌 Initializing GPIO pins...");
    for (int pin : ledPins) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }
    for (int pin : shadePins) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }
    Serial.println("✅ GPIO pins initialized");
    
    initPIRSensor();
    
    Serial.println("🌐 Connecting to WiFi...");
    connectToWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("📡 Setting up MQTT client...");
      client.setServer(mqtt_server.c_str(), mqtt_port);
      client.setCallback(callback);
      connectToMQTT();
    } else {
      Serial.println("❌ WiFi connection failed, cannot connect to MQTT");
    }
    
    Serial.println("\n💡 Tip: Type 'show' in Serial Monitor to view current configuration");
  }
}

void loop() {
  if (configMode) {
    handleSerialInput();
    checkConfigTimeout();
    return;
  }
  
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    String command = input;
    command.toLowerCase();
    
    if (command == "show") {
      showCurrentConfig();
    }
  }
  
  if (!client.connected()) {
    Serial.println("⚠️ MQTT disconnected, attempting to reconnect...");
    connectToMQTT();
  }
  client.loop();

  // Check PIR sensor motion continuously (every 1 second for responsiveness)
  static uint32_t pirTimer = millis();
  if (millis() - pirTimer > 1000) {  // Check every 1 second
    pirTimer = millis();
    checkPIRMotion();
  }

  if (millis() - lastPingTime >= pingInterval) {
    sendPing();
    lastPingTime = millis();
  }
}
