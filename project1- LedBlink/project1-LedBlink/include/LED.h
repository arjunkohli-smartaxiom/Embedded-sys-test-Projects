// LED.h
#ifndef LED_H
#define LED_H

class LED {
public:
    LED(int pin); // Constructor
    void on();    // Turn the LED on
    void off();   // Turn the LED off
    void toggle(); // Toggle the LED state

private:
    int pin; // Pin number where the LED is connected
};

#endif
