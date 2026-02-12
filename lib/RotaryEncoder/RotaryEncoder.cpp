#include "RotaryEncoder.h"

// Static instance for ISR
static RotaryEncoder* instancePointer = nullptr;

// State transition table
// Indexed by: (prevState << 2) | currentState
// Returns: -1 (CCW), 0 (invalid), +1 (CW)
const int8_t RotaryEncoder::stateTable[16] = {
     0,  // 0000: no change
    -1,  // 0001: CCW step
    +1,  // 0010: CW step
     0,  // 0011: invalid
    +1,  // 0100: CW step
     0,  // 0101: no change
     0,  // 0110: invalid
    -1,  // 0111: CCW step
    -1,  // 1000: CCW step
     0,  // 1001: invalid
     0,  // 1010: no change
    +1,  // 1011: CW step
     0,  // 1100: invalid
    +1,  // 1101: CW step
    -1,  // 1110: CCW step
     0   // 1111: no change
};

RotaryEncoder::RotaryEncoder(int clk, int dt)
    : pinCLK(clk), pinDT(dt), encoderState(0), position(0), lastInterruptTime(0) {
    instancePointer = this;
}

void RotaryEncoder::begin() {
    pinMode(pinCLK, INPUT_PULLUP);
    pinMode(pinDT, INPUT_PULLUP);
    
    // Read initial state
    int clk = digitalRead(pinCLK);
    int dt = digitalRead(pinDT);
    encoderState = (clk << 1) | dt;
    
    // Attach interrupts to BOTH pins
    attachInterrupt(digitalPinToInterrupt(pinCLK), handleInterruptStatic, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinDT), handleInterruptStatic, CHANGE);
    
    Serial.println("[ENC] Initialized with state machine decoder");
}

void IRAM_ATTR RotaryEncoder::handleInterruptStatic() {
    if (instancePointer) {
        instancePointer->updatePosition();
    }
}

void RotaryEncoder::updatePosition() {
    unsigned long currentTime = millis();
    
    // Aggressive debouncing
    if (currentTime - lastInterruptTime < DEBOUNCE_DELAY) {
        return;  // Too fast, ignore
    }
    lastInterruptTime = currentTime;
    
    // Read current state
    int clk = digitalRead(pinCLK);
    int dt = digitalRead(pinDT);
    int currentState = (clk << 1) | dt;
    
    // Look up state transition
    int index = (encoderState << 2) | currentState;
    int8_t direction = stateTable[index];
    
    if (direction != 0) {
        position += direction;
        Serial.printf("[ENC] %s (state: %d -> %d)\n", 
                     (direction > 0) ? "CW" : "CCW",
                     encoderState, currentState);
    }
    
    // Update state for next transition
    encoderState = currentState;
}

int RotaryEncoder::getDirection() {
    noInterrupts();
    
    int currentPosition = position;
    
    if (abs(currentPosition) >= 4) {
        int detents = currentPosition / 4;
        
        position = currentPosition % 4;
        
        interrupts();
        
        return detents;
    }
    
    interrupts();
    return 0;
}
