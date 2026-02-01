#include "Button.h"
#include <Arduino.h>

// ==========================================
// 2. CLASS: BUTTON WITH SHORT/LONG PRESS DETECTION
//===========================================

    Button::Button(int p, unsigned long longPressTime) {
      pin = p;
      longPressThreshold = longPressTime;
      wasPressed = false;
      longPressHandled = false;
      pressStartTime = 0;
      shortPressReady = false;
    }

    void Button:: begin() {
    pinMode(pin, INPUT_PULLUP);
}

    void Button::update() {
      int reading = digitalRead(pin);
      if (reading == LOW && !wasPressed) {
        delay(10); // Debounce
        reading = digitalRead(pin);
        if (reading == LOW) {
            wasPressed = true;
        pressStartTime = millis();
        longPressHandled = false;
        shortPressReady = false;
        }
      }
      if (wasPressed && reading == LOW) {
        if ((millis() - pressStartTime > longPressThreshold) && !longPressHandled) {
          longPressHandled = true;
        }
      }
      if (reading == HIGH && wasPressed) {
        wasPressed = false;
        if (!longPressHandled) {
          shortPressReady = true;
        }
      }
    }

    bool Button::wasShortPressed() {
      if (shortPressReady) {
        shortPressReady = false; 
        return true;
      }
      return false;
    }

    bool Button::wasLongPressed() {
      if (longPressHandled && !wasPressed) {
        longPressHandled = false; 
        return true;
      }
      return false;
    }

    bool Button::isPressed() {
      return wasPressed;
    }