#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <Arduino.h>
#include "Waveforms/WaveformGenerator.h"

class StateMachine;  // Forward declaration
class Potentiometer; // Forward declaration

class AudioEngine {
private:
    // I2S Configuration
    static const int SAMPLE_RATE = 44100;
    static const int BUFFER_SIZE = 512;
     int I2S_BCK_PIN;
    int I2S_LRCK_PIN;
    int I2S_DIN_PIN;

    // Audio buffer
    int16_t audioBuffer[BUFFER_SIZE];

    // Waveform synthesis
    WaveformGenerator* currentWaveform;
    float phase;
    float frequency;        
    float phaseIncrement;
    float amplitude;
    float masterVolume;

    WaveformGenerator* waveforms[5]; // ← Array to hold different waveform generators

    // Audio state (for feedback tone)
    enum AudioState {
        NORMAL_PLAYBACK,
        FEEDBACK_TONE
    };
    AudioState audioState;          
    int feedbackSamplesRemaining;   
    float feedbackFrequency;        

public:
    AudioEngine(int bck, int lrck, int din);  // ← Constructor

    void begin();
    void update(const StateMachine &stateMachine, const Potentiometer &potPitch, const Potentiometer &potTone);
    
    void setWaveform(WaveformGenerator* waveform);
    void setFrequency(float freq);     
    void setAmplitude(float amp);      
    void setMasterVolume(float vol);   
    
    void playFeedbackTone(float frequency, int durationMs);

private:
    void fillBuffer();             
    void updatePhaseIncrement();  
    void fillFeedbackBuffer(); 
};

#endif