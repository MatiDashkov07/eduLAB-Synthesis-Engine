#include "AudioEngine.h"
#include "StateMachine/StateMachine.h"
#include "Potentiometer/Potentiometer.h"
#include <Arduino.h>

AudioEngine::AudioEngine(int pin)
    : buzzerPin(pin), lastAppliedFreq(0), lastAppliedDuty(-1), forceUpdate(false), lastNoiseUpdate(0) {
}

void AudioEngine::begin() {
    ledcSetup(LEDC_CHANNEL, BASE_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(buzzerPin, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0); 
}

void AudioEngine::update(const StateMachine &stateMachine, const Potentiometer &potPitch, const Potentiometer &potTone) {
    StateMachine::State currentState = stateMachine.getState();

    if (currentState == StateMachine::MUTE) {
        mute();
        return;
    }
    
    if(currentState == StateMachine::MENU) {
        return;
    }

    const Menu &menu = stateMachine.getMenu();
    int selectedMode = menu.getSelectedMode();

    if (selectedMode == -1) {
        mute();
        return;
    }

    if (selectedMode == Menu::NOISE) {
        if (millis() - lastNoiseUpdate > 5) { 
            generateNoise(potPitch.getValue());
            lastNoiseUpdate = millis();
        }
    } 
    else {
        int targetFrequency = map(potPitch.getValue(), 0, 4095, MIN_FREQ_SAFE, 2000);
        int targetDuty = map(potTone.getValue(), 0, 4095, 0, 255); 

        if (targetFrequency != lastAppliedFreq || targetDuty != lastAppliedDuty || forceUpdate) {
            generateSquare(targetFrequency, targetDuty);
            forceUpdate = false;
        }
        
    }
}    


void AudioEngine::playFeedbackTone(int frequency, int duration) {
    ledcChangeFrequency(LEDC_CHANNEL, frequency, LEDC_RESOLUTION);
    ledcWrite(LEDC_CHANNEL, 128); 
    delay(duration);
    ledcWrite(LEDC_CHANNEL, 0); 
    forceUpdate = true; 
    lastAppliedFreq = 0; 
}

void AudioEngine::generateSquare(int frequency, int duty) {
    if (frequency != lastAppliedFreq) {
        ledcChangeFrequency(LEDC_CHANNEL, frequency, LEDC_RESOLUTION);
        lastAppliedFreq = frequency;
    }
    if (duty != lastAppliedDuty) {
        ledcWrite(LEDC_CHANNEL, duty);
        lastAppliedDuty = duty;
    }
}

void AudioEngine::generateNoise(int pitchValue) {
    int noiseCeiling = map(pitchValue, 0, 4095, 600, 8000);
    if (noiseCeiling < 600) noiseCeiling = 600;

    int noiseFreq = random(200, noiseCeiling);
    int noiseDuty = random(10, 255);

    ledcChangeFrequency(LEDC_CHANNEL, noiseFreq, LEDC_RESOLUTION);
    ledcWrite(LEDC_CHANNEL, noiseDuty); 

    lastAppliedFreq = noiseFreq;
    lastAppliedDuty = noiseDuty;
}

void AudioEngine::mute() {
    if (lastAppliedDuty != 0) {
        ledcWrite(LEDC_CHANNEL, 0);
        lastAppliedDuty = 0;
    }
}

