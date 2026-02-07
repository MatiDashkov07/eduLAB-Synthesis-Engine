#include <Arduino.h>
#include "StateMachine.h"
#include "DisplayManager.h"
#include "AudioEngine.h"
#include "Button.h"
#include "RotaryEncoder.h"
#include "Potentiometer.h"

// ==========================================
// HARDWARE CONFIGURATION
// ==========================================
const int SW_PIN        = 15;
const int POT_PIN_PITCH = 1;
const int POT_PIN_TONE  = 2;
const int PIN_DT        = 7;
const int PIN_CLK       = 6;
const int I2S_BCK_PIN = 38;
const int I2S_DIN_PIN = 39;
const int I2S_LRCK_PIN = 40;

// ==========================================
// OBJECT INSTANCES
// ==========================================
StateMachine stateMachine;
DisplayManager displayManager;
AudioEngine audioEngine{I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DIN_PIN};

Button button(SW_PIN);
RotaryEncoder encoder(PIN_CLK, PIN_DT);
Potentiometer potPitch(POT_PIN_PITCH);
Potentiometer potTone(POT_PIN_TONE);

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(500);
    
    // Initialize hardware
    button.begin();
    encoder.begin();
    potPitch.begin();
    potTone.begin();
    
    displayManager.begin();  // I2C + OLED + Splash screen
    audioEngine.begin();    // I2S + Audio setup
    
    Serial.println("eduLAB v3.8 - OOP Refactored");
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
    // 1. UPDATE INPUTS
    button.update();
    //encoder.update();      // Not needed with interrupt-based encoder
    potPitch.update();
    potTone.update();
    
    // 2. HANDLE BUTTON EVENTS
    if (button.wasLongPressed()) {
        stateMachine.onButtonLongPress();
        audioEngine.playFeedbackTone(500, 100);
    }
    
    if (button.wasShortPressed()) {
        stateMachine.onButtonShortPress();
        audioEngine.playFeedbackTone(2000, 50);
    }
    
    // 3. HANDLE ENCODER MOVEMENT
    int direction = encoder.getDirection();
    if (direction != 0) {
        stateMachine.onEncoderMoved(direction);
    }
    
    // 4. UPDATE STATE MACHINE (timeout check)
    stateMachine.update();
    
    // 5. CALCULATE FREQUENCY FOR DISPLAY
    int selectedMode = stateMachine.getMenu().getSelectedMode();
    int maxFreq = (selectedMode == Menu::NOISE) ? 5000 : 20000;
    int currentFrequency = map(potPitch.getValue(), 0, 4095, 20, maxFreq);
    
    // 6. UPDATE OUTPUTS
    displayManager.update(stateMachine, currentFrequency);
    audioEngine.update(stateMachine, potPitch, potTone);
}