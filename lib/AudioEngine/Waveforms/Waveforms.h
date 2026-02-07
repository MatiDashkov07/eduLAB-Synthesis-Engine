#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include "WaveformGenerator.h"
#include <Arduino.h>

// ========== Sine Wave ==========
class SineWave : public WaveformGenerator {
public:
    float getSample(float phase) override {
        return sin(phase);
    }
};

// ========== Triangle Wave ==========
class TriangleWave : public WaveformGenerator {
public:
    float getSample(float phase) override {
        // Triangle: -1 to 1, symmetric
        if (phase < PI) {
            return -1.0f + (2.0f * phase / PI);  // Rising: -1 → 1
        } else {
            return 3.0f - (2.0f * phase / PI);   // Falling: 1 → -1
        }
    }
};

// ========== Square Wave ==========
class SquareWave : public WaveformGenerator {
public:
    float getSample(float phase) override {
        return (phase < PI) ? 1.0f : -1.0f;
    }
};

// ========== Sawtooth Wave ==========
class SawWave : public WaveformGenerator {
public:
    float getSample(float phase) override {
        return -1.0f + (phase / PI);  // Linear rise: -1 → 1
    }
};

// ========== Noise ==========
class NoiseWave : public WaveformGenerator {
public:
    float getSample(float phase) override {
        return random(-32767, 32767) / 32767.0f;  // Random: -1 to 1
    }
};

#endif
