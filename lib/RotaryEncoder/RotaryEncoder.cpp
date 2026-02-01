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
    if (currentTime - lastInterruptTime < 5) {
        return; // Debounce: ignore interrupts within 5ms
    }
    lastInterruptTime = currentTime;

    if (digitalRead(pinCLK) != digitalRead(pinDT)) {
        position++;
    } else {
        position--;
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