#ifdef TEENSY_BUILD

#include "AudioEngine.h"
#include <imxrt.h>

void AudioEngine::begin() {
    //Step 1: Enable SAI1 clock
    CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

    // Step 2: TODO - PLL configuration
    // Step 3: TODO - SAI1 registers
    // Step 4: TODO - DMA setup
    // Step 5: TODO - Pin mux
}

#endif