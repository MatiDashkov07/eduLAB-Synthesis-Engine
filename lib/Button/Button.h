#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button 
{
    private:
        int pin;
        unsigned long pressStartTime;
        unsigned long longPressThreshold;
        bool wasPressed;           // state tracking
        bool longPressHandled;
        bool longPressTriggered;
        bool shortPressReady;

    public:
        Button(int p, unsigned long longPressTime = 800);
        void begin();                   // initializes the pin
        void update();                  // reads the pin and updates state
        bool wasShortPressed();         // returns true once if there was a short press
        bool wasLongPressed();          // returns true once if there was a long press
        bool isPressed();               // returns the current state
};




#endif