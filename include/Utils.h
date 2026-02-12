#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

/**
 * Standard logarithmic frequency mapping for audio control
 * 
 * Maps ADC value (0-4095) to frequency (minFreq-maxFreq) logarithmically.
 * Provides fine control at low frequencies, coarse at high frequencies.
 * 
 * @param adcValue: Raw ADC reading (0-4095)
 * @param minFreq: Minimum frequency (Hz)
 * @param maxFreq: Maximum frequency (Hz)
 * @return Mapped frequency (Hz)
 */
inline float mapLogarithmic(int adcValue, float minFreq, float maxFreq) {
    float t = constrain(adcValue, 0, 4095) / 4095.0f;
    float ratio = maxFreq / minFreq;
    return minFreq * pow(ratio, t);
}

/**
 * Logarithmic mapping with dead zones at extremes
 * 
 * Snaps to exact min/max when ADC is near endpoints.
 * Compensates for:
 * - ESP32 ADC non-linearity (doesn't reach exact 0/4095)
 * - EMA filter lag (filtered value lags behind actual pot position)
 * 
 * @param adcValue: Raw ADC reading (0-4095)
 * @param minFreq: Minimum frequency (Hz)
 * @param maxFreq: Maximum frequency (Hz)
 * @param deadZone: ADC units from edge to snap (default: 100 = ~2.4% of range)
 * @return Mapped frequency (Hz)
 */
inline float mapLogarithmicWithDeadZone(int adcValue, float minFreq, float maxFreq, int deadZone = 100) {
    // Dead zone at minimum
    if (adcValue <= deadZone) {
        return minFreq;
    }
    
    // Dead zone at maximum
    if (adcValue >= (4095 - deadZone)) {
        return maxFreq;
    }
    
    // Normal logarithmic mapping in the middle
    float t = constrain(adcValue, 0, 4095) / 4095.0f;
    float ratio = maxFreq / minFreq;
    return minFreq * pow(ratio, t);
}

/**
 * Asymmetric dead zone version
 * 
 * Smaller dead zone at low end (gradual transition OK)
 * Larger dead zone at high end (avoid big jump)
 * 
 * Use this if the jump to maxFreq is too jarring.
 */
inline float mapLogarithmicAsymmetric(int adcValue, float minFreq, float maxFreq) {
    // Small dead zone at minimum
    if (adcValue <= 50) {
        return minFreq;
    }
    
    // Larger dead zone at maximum
    if (adcValue >= 3945) {
        return maxFreq;
    }
    
    // Normal mapping
    float t = constrain(adcValue, 0, 4095) / 4095.0f;
    float ratio = maxFreq / minFreq;
    return minFreq * pow(ratio, t);
}

#endif