#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512
#define REGISTRATION_FLAG_ADDR 0

const char* ssid = "robozzlab";
const char* password = "robozz123";

const char* apiURL = "https://eoozi7gkvqwc4fd.m.pipedream.net";

bool isRegistered = false;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  if (EEPROM.read(REGISTRATION_FLAG_ADDR) == 1) {
    isRegistered = true;
    Serial.println("Device is already registered.");
  } else {
    Serial.println("Device not registered. Starting registration flow...");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  String deviceMAC = WiFi.macAddress();
  Serial.println("Device MAC: " + deviceMAC);

  if (!isRegistered) {
    registerDevice(deviceMAC);
  }
}

void registerDevice(String mac) {
  HTTPClient http;
  http.begin(apiURL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"mac\":\"" + mac + "\"}";
  Serial.println("Sending payload: " + payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Device Registered Successfully.");
    Serial.println("Server response: " + response);

    EEPROM.write(REGISTRATION_FLAG_ADDR, 1);
    EEPROM.commit();
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void loop() {
  // your main logic
}
