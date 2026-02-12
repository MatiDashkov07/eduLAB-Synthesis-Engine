#include "RotaryEncoder.h"
#include <Arduino.h>

// Initialize static instance pointer
RotaryEncoder* RotaryEncoder::instance = nullptr;

RotaryEncoder::RotaryEncoder(int clkPin, int dtPin)
    : pinCLK(clkPin), pinDT(dtPin), position(0), lastReadPosition(0), lastInterruptTime(0) {
}

void RotaryEncoder::begin() {
    pinMode(pinCLK, INPUT);
    pinMode(pinDT, INPUT);
    instance = this; // Set the static instance pointer
    attachInterrupt(digitalPinToInterrupt(pinDT), RotaryEncoder::staticWrapper, FALLING);
}

void RotaryEncoder::staticWrapper() {
    if (instance) {
        instance->updatePosition();
    }
}

void RotaryEncoder::updatePosition() {
    unsigned long currentTime = millis();
    
    // Debounce check
    if (currentTime - lastInterruptTime < 5) {
        Serial.println("[ENC] Debounced (too fast)");  // ← Debug
        return;
    }
    lastInterruptTime = currentTime;

    // Read both pins
    int clkState = digitalRead(pinCLK);
    int dtState = digitalRead(pinDT);
    
    // Determine direction
    if (clkState != dtState) {
        position++;
        Serial.println("[ENC] CW +1");  // ← Debug
    } else {
        position--;
        Serial.println("[ENC] CCW -1");  // ← Debug
    }
}

void RotaryEncoder::resetPosition() {
    position = 0;
    lastReadPosition = 0;
}

int RotaryEncoder::getPosition() {
    noInterrupts();
    int pos = position;
    interrupts();
    return pos;
}

int RotaryEncoder::getDirection() {
    noInterrupts();
    int currentPos = position;
    interrupts();
    int direction = currentPos - lastReadPosition;
    lastReadPosition = currentPos;
    if (direction > 0) return 1;
    if (direction < 0) return -1;
    return 0;
}