#ifndef ROTARYENCODER_H
#define ROTARYENCODER_H

#include <Arduino.h>

class RotaryEncoder
{
private:
    int pinCLK;
    int pinDT;
    volatile int position;
    int lastReadPosition;
    volatile unsigned long lastInterruptTime;
    static RotaryEncoder* instance; 
    
    void updatePosition();          // ← real ISR logic
    static void staticWrapper();    // ← static wrapper for attachInterrupt
    
public:
    RotaryEncoder(int clkPin, int dtPin);
    void begin();
    int getPosition();
    void resetPosition();
    int getDirection();
};

#endif