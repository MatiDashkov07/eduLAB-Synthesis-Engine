#ifndef CONSTS_H
#define CONSTS_H

static const float SAMPLE_RATE = 44117.64706f; // Legacy Teensy 3.x audio rate (48 MHz / 1088); AudioEngine::begin() programs the audio PLL from it (~44116.9 Hz actual)
static const int BUFFER_SIZE = 256;

#endif