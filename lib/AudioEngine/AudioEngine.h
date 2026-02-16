#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <Arduino.h>
#include "Waveforms/WaveformGenerator.h"
#include "Voice.h"
#include "../include/Consts.h"

class StateMachine;  // Forward declaration
class Potentiometer; // Forward declaration

class AudioEngine {
    ;
private:
    // I2S Configuration
    int I2S_BCK_PIN;
    int I2S_LRCK_PIN;
    int I2S_DIN_PIN;

    // Audio buffer
    int16_t audioBuffer[BUFFER_SIZE];

    // Waveform synthesis
    //WaveformGenerator* currentWaveform;
    //float phase;
    //float frequency;        
    //float phaseIncrement;
    //float amplitude;
    float masterVolume;

    WaveformGenerator* waveforms[5]; // ← Array to hold different waveform generators

    Voice voices[4]; // ← Array of voices for polyphony

    // Audio state (for feedback tone)
    enum AudioState {
        NORMAL_PLAYBACK,
        FEEDBACK_TONE
    };
    volatile AudioState audioState;          
    volatile int feedbackSamplesRemaining;   
    volatile float feedbackFrequency;        

public:
    AudioEngine(int bck, int lrck, int din);  // ← Constructor

    void begin();
    void update(const StateMachine &stateMachine, const Potentiometer &potPitch, const Potentiometer &potTone);
    
    void setWaveform(int voiceIndex, WaveformGenerator* waveform);
    void setFrequency(int voiceIndex, float freq);     
    void setAmplitude(int voiceIndex, float amp);      
    void setMasterVolume(float vol);   
    void noteOn(int voiceIndex, float freq, float amp);
    void noteOff(int voiceIndex);
    
    void playFeedbackTone(float frequency, int durationMs);

private:
    void fillBuffer();             
    //void updatePhaseIncrement();  
    void fillFeedbackBuffer(); 
};

#endif