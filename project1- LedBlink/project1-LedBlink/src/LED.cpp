// LED.cpp
#include "LED.h"
#include <Arduino.h> // Include Arduino library for digitalWrite

LED::LED(int pin) {
    this->pin = pin;
    pinMode(pin, OUTPUT); // Set the pin as output
}

void LED::on() {
    digitalWrite(pin, HIGH); // Turn the LED on
}

void LED::off() {
    digitalWrite(pin, LOW); // Turn the LED off
}

void LED::toggle() {
    digitalWrite(pin, !digitalRead(pin)); // Toggle the LED state
}
