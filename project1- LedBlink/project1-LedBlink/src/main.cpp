// main.cpp
#include <Arduino.h>
#include "LED.h" // Include your custom library

LED myLED(2); // Create an instance of the LED class on pin 13

void setup() {
    Serial.begin(9600); // Initialize serial communication
    Serial.println("LED Blink Example");
}

void loop() {
    myLED.on();      // Turn on the LED
    Serial.println("LED is ON");
    delay(1000);     // Wait for 1 second

    myLED.off();     // Turn off the LED
    Serial.println("LED is OFF");
    delay(1000);     // Wait for 1 second
}
