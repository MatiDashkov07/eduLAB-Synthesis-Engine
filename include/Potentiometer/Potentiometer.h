#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <Arduino.h>

class Potentiometer 
{
    private:
        int pin;
        float filteredValue;
        float alpha; 
        int lastStableValue;
        int threshold; 
    
    public:
        // Alpha 0.15 = faster response
        // Threshold 40 = more aggressive filtering of Breadboard noise
        Potentiometer(int p, float a = 0.15, int th = 40); 
        void begin();
        bool update();
        int getValue();
    };

#endif