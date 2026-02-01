#include "Potentiometer.h"
#include <Arduino.h>

// ==========================================
// 2. CLASS: SMOOTH POTENTIOMETER (EMA FILTER)
// ==========================================

    Potentiometer::Potentiometer(int p, float a, int th) { 
      pin = p;
      alpha = a;
      threshold = th;
      filteredValue = 0;
      lastStableValue = 0;
    }

    void Potentiometer::begin() {
       pinMode(pin, INPUT);
       filteredValue = analogRead(pin);
       lastStableValue = filteredValue;
    }

    bool Potentiometer::update() {
      int raw = analogRead(pin);
      
      // EMA Filter
      filteredValue = (filteredValue * (1.0 - alpha)) + (raw * alpha);

      // Hysteresis Check
      if (abs((int)filteredValue - lastStableValue) > threshold) {
        lastStableValue = (int)filteredValue;
        return true; 
      }
      return false; 
    }

    int Potentiometer::getValue() const{
      return lastStableValue;
    }
