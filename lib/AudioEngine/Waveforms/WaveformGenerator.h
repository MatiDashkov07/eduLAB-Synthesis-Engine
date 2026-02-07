#ifndef WAVEFORM_GENERATOR_H
#define WAVEFORM_GENERATOR_H
#include <Arduino.h>

class WaveformGenerator {
public:
    virtual float getSample(float phase) = 0; // Pure virtual function to get the next sample
    virtual ~WaveformGenerator() {} // Virtual destructor
};
#endif