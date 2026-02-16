#ifndef VOICE_H
#define VOICE_H

#include "Waveforms/WaveformGenerator.h"
#include "../include/Consts.h"

class Voice {
private:
    WaveformGenerator* waveform;
    float frequency;
    float amplitude;
    float phase;
    float phaseIncrement;
    bool isActive;

public:
    Voice(WaveformGenerator* wf = nullptr, float freq = 0.0f, float amp = 0.0f) 
        : waveform(wf), frequency(freq), amplitude(amp), phase(0), isActive(false) {}
    
    float getNextSample() {}
    void noteOn(float freq, float amp) {}
    void noteOff() {}
    void setWaveform(WaveformGenerator* wf) {}
    void setFrequency(float freq) {}
    void setAmplitude(float amp) {}
    bool getIsActive() const { return isActive; }


private:
    void updatePhaseIncrement() {}
};
#endif