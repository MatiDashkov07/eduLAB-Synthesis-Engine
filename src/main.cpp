#include <Arduino.h>
#ifdef TEENSY_BUILD

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Pin 13
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500); // On for 500ms
  digitalWrite(LED_BUILTIN, LOW);
  delay(500); // Off for 500ms
}

#else


#include "StateMachine.h"
#include "DisplayManager.h"
#include "AudioEngine.h"
#include "Button.h"
#include "RotaryEncoder.h"
#include "Potentiometer.h"
#include "../include/Utils.h"

// FreeRTOS headers for task management (built into ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==========================================
// HARDWARE CONFIGURATION
// ==========================================
const int SW_PIN        = 15;
const int POT_PIN_PITCH = 1;
const int POT_PIN_TONE  = 2;
const int PIN_DT        = 7;
const int PIN_CLK       = 6;
const int I2S_BCK_PIN = 39;
const int I2S_DIN_PIN = 40;
const int I2S_LRCK_PIN = 38;

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
// DUAL-CORE ARCHITECTURE
// ==========================================

// Task handle for audio task
TaskHandle_t audioTaskHandle;

/**
 * Audio Task - Runs on Core 0 (dedicated)
 * 
 * This task runs continuously and ONLY handles audio generation.
 * It has HIGH priority to ensure glitch-free audio output.
 * 
 * Core 0 is typically less loaded (WiFi/BT disabled in this project),
 * making it perfect for real-time audio.
 * 
 * @param parameter - Unused (required by FreeRTOS signature)
 */
void audioTask(void* parameter) {
    Serial.println("[Audio Task] Started on Core 0");
    
    // Infinite loop - like loop() but dedicated to audio
    while (true) {
        // This is where the magic happens:
        // - Reads shared variables (frequency, amplitude)
        // - Generates audio samples
        // - Sends buffer to I2S DAC
        // 
        // The i2s_write() inside update() is BLOCKING,
        // which naturally rate-limits this task to ~5.8ms intervals
        // (the time it takes to play 256 samples @ 44.1kHz)
        
        audioEngine.update(stateMachine, potPitch, potTone);
        
        // Note: No vTaskDelay() needed here because i2s_write() blocks.
        // The DMA dictates our timing, which is exactly what we want!
    }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("eduLAB v4.0 - Dual-Core Architecture");
    Serial.println("=====================================");
    
    // Initialize hardware
    button.begin();
    encoder.begin();
    potPitch.begin();
    potTone.begin();
    
    displayManager.begin();  // I2C + OLED + Splash screen
    audioEngine.begin();     // I2S + Audio setup
    
    // Create audio task on Core 0
    xTaskCreatePinnedToCore(
        audioTask,           // Task function
        "AudioEngineTask",   // Name (for debugging)
        10000,               // Stack size in bytes (10KB is plenty)
        NULL,                // Task parameters (none)
        2,                   // Priority (2 = high, higher than default loop)
        &audioTaskHandle,    // Task handle (for future control if needed)
        0                    // Core ID: 0 (dedicated audio core)
    );
    
    Serial.println("[Setup] Core 0: Audio Task (High Priority)");
    Serial.println("[Setup] Core 1: UI/Display Loop (Normal Priority)");
    Serial.println("=====================================");
}

// ==========================================
// MAIN LOOP - Runs on Core 1
// ==========================================
void loop() {
    // ==========================================
    // This loop now ONLY handles UI and controls
    // Audio runs independently on Core 0
    // ==========================================
    
    // 1. UPDATE INPUTS
    button.update();
    potPitch.update();
    potTone.update();
    
    // 2. HANDLE BUTTON EVENTS
    if (button.wasLongPressed()) {
        stateMachine.onButtonLongPress();
        audioEngine.playFeedbackTone(500, 100);
    }
    
    if (button.wasShortPressed()) {
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
    
    // 5. CALCULATE FREQUENCY FOR DISPLAY
    int selectedMode = stateMachine.getMenu().getSelectedMode();
    int maxFreq = (selectedMode == Menu::NOISE) ? 5000 : 20000;
    float currentFrequency = mapLogarithmicAsymmetric(potPitch.getValue(), 20.0f, maxFreq);
    
    // 6. UPDATE DISPLAY
    // This can now run without affecting audio!
    // The I2C communication (which blocks for ~10ms) happens on Core 1,
    // while Core 0 continues generating audio smoothly.
    displayManager.update(stateMachine, (int)currentFrequency);
    
    // 7. AUDIO UPDATE - REMOVED!
    // audioEngine.update() has been moved to Core 0 (audioTask)
    // This separation is what eliminates the clicks/pops!
    
    // Small delay to yield CPU to other tasks (good practice)
    delay(10);
}

#endif