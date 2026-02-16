#include "Voice.h"
#include "Waveforms/Waveforms.h"
#include "../include/Consts.h"
#include <Arduino.h>

Voice::Voice(WaveformGenerator* wf, float freq, float amp) 
    : waveform(wf), frequency(freq), amplitude(amp), phase(0), isActive(false) {
    updatePhaseIncrement();
}

float Voice::getNextSample() {
    if (!isActive || !waveform) {
        return 0.0f;
    }

    float sample = waveform->getSample(phase) * amplitude;
    
    phase += phaseIncrement;
    if (phase >= TWO_PI) {
        phase -= TWO_PI;
    }

    return sample;
}

void Voice::noteOn(float freq, float amp) {
    phase = 0; // Reset phase for new note
    frequency = freq;
    amplitude = amp;
    isActive = true;
    updatePhaseIncrement();
}

void Voice::noteOff() {
    isActive = false;
}

void Voice::setWaveform(WaveformGenerator* wf) {
    waveform = wf;
}

void Voice::setFrequency(float freq) {
    frequency = freq;
    updatePhaseIncrement();
}

void Voice::setAmplitude(float amp) {
    amplitude = amp;
}

void Voice::updatePhaseIncrement() {
    phaseIncrement = TWO_PI * frequency / SAMPLE_RATE;
}