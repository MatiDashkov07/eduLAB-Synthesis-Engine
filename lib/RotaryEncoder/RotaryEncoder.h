#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

class RotaryEncoder {
public:
    RotaryEncoder(int clk, int dt);
    void begin();
    int getDirection();  // Returns: -1 (CCW), 0 (none), +1 (CW)

private:
    int pinCLK;
    int pinDT;
    
    // State machine approach
    volatile int8_t encoderState;
    volatile int position;
    
    // Timing
    volatile unsigned long lastInterruptTime;
    static const unsigned long DEBOUNCE_DELAY = 2;  // ms
    
    // ISR helper
    static void IRAM_ATTR handleInterruptStatic();
    void updatePosition();
    
    // Lookup table for state machine
    static const int8_t stateTable[16];
};

#endif