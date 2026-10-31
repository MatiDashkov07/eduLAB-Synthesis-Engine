#include <Arduino.h>
#ifdef TEENSY_BUILD


#include "AudioEngine.h"
#include "Potentiometer.h"
#include  "StateMachine.h"
#include "RotaryEncoder.h"
#include "Button.h"
#include "DisplayManager.h"
#include "../include/Utils.h"


// ==========================================
// HARDWARE CONFIGURATION
// ==========================================


const int I2S_BCK_PIN = 21;
const int I2S_LRCK_PIN = 20;
const int I2S_DIN_PIN = 7;
const int POT_PIN_PITCH = 14;
const int POT_PIN_AMP  = 15;
const int PIN_SW = 35;
const int PIN_CLK = 37;
const int PIN_DT = 36;


// ==========================================
// OBJECT INSTANCES
// ==========================================


AudioEngine audioEngine(I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DIN_PIN);
Potentiometer potPitch(POT_PIN_PITCH);
Potentiometer potAmp(POT_PIN_AMP);
StateMachine stateMachine;
RotaryEncoder encoder(PIN_CLK, PIN_DT);
Button button(PIN_SW);
DisplayManager displayManager;


// ==========================================
// SETUP
// ==========================================


void setup() {
    Serial.begin(115200);

    delay(2000);

    if (CrashReport) {
        Serial.print(CrashReport);
        delay(5000);
    }


    audioEngine.begin();
    audioEngine.setMasterVolume(1.0f);
    audioEngine.noteOn(0, 440.0f, 1.0f);

    analogReadResolution(12);

    displayManager.begin();
    potPitch.begin();
    potAmp.begin();
    encoder.begin();
    button.begin();

}


// ==========================================
// MAIN LOOP
// ==========================================


void loop() {        
    // 1. UPDATE INPUTS
    button.update();
    potPitch.update();
    potAmp.update();
    
    // 2. HANDLE BUTTON EVENTS
    if (button.wasLongPressed()) {
        Serial.println("LONG PRESS DETECTED");
        stateMachine.onButtonLongPress();
        Serial.printf("State after: %d\n", stateMachine.getState());
        audioEngine.playFeedbackTone(500, 100);
    }
    
    if (button.wasShortPressed()) {
        Serial.println("SHORT PRESS DETECTED");
        stateMachine.onButtonShortPress();
        
        // Mute/Unmute gets a distinctive "double beep"
        if (stateMachine.getState() == StateMachine::MUTE) {
            // Entering mute: LOW tone, longer
            audioEngine.playFeedbackTone(300, 150);  // 150ms, low pitch
        } else {
            // Exiting mute: HIGH tone, longer  
            audioEngine.playFeedbackTone(1500, 150);  // 150ms, high pitch
        }
    }
    
    // 3. HANDLE ENCODER MOVEMENT
    int direction = encoder.getDirection();
    if (direction != 0) {
        stateMachine.onEncoderMoved(direction);
    }
    
    // 4. UPDATE STATE MACHINE (timeout check)
    stateMachine.update();

    // 5. UPDATE AUDIO ENGINE with latest state and potentiometer values
    audioEngine.update(stateMachine, potPitch, potAmp);

    int maxFreq = 20000;
    float currentFrequency = mapLogarithmicAsymmetric(potPitch.getValue(), 20.0f, maxFreq);
    
    // 6. UPDATE DISPLAY
    displayManager.update(stateMachine, (int)currentFrequency);

    delay(10); // Small delay to prevent overwhelming the CPU with state updates
}

#endif
