#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <Arduino.h>

class StateMachine;
class Potentiometer;

class AudioEngine {
private:
    static const int LEDC_CHANNEL = 0;
    static const int LEDC_RESOLUTION = 8;
    static const int BASE_FREQ = 5000;
    static const int MIN_FREQ_SAFE = 350;

    int buzzerPin;

    int lastAppliedFreq;
    int lastAppliedDuty;
    bool forceUpdate;
    unsigned long lastNoiseUpdate;

public:
    AudioEngine(int pin);
    void begin();
    void update(const StateMachine &stateMachine, const Potentiometer &potPitch, const Potentiometer &potTone);

    void playFeedbackTone(int frequency, int duration);

private:
    void generateSquare(int frequency, int duty);
    void generateNoise(int pitchValue);
    void mute();
};

#endif 