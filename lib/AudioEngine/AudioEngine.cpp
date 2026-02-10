#include "AudioEngine.h"
#include "StateMachine.h"
#include "Potentiometer.h"
#include <Arduino.h>
#include "driver/i2s.h"
#include "Waveforms/Waveforms.h"


AudioEngine::AudioEngine(int bck, int lrck, int din)
    : currentWaveform(nullptr), phase(0), frequency(440), amplitude(1.0), masterVolume(0.5),
      audioState(NORMAL_PLAYBACK), feedbackSamplesRemaining(0), feedbackFrequency(0),
      I2S_BCK_PIN(bck), I2S_LRCK_PIN(lrck), I2S_DIN_PIN(din) {
}


void AudioEngine::begin() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, 
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, 
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, 
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, 
        .dma_buf_count = 8, 
        .dma_buf_len = 64, 
        .use_apll = false, 
        .tx_desc_auto_clear = true 
    };

    i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LRCK_PIN,   
    .data_out_num = I2S_DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE 
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);

    waveforms[0] = new SineWave();
    waveforms[1] = new TriangleWave();
    waveforms[2] = new SquareWave();
    waveforms[3] = new SawWave();
    waveforms[4] = new NoiseWave();

    currentWaveform = waveforms[0];

    Serial.println("I2S Initialized!");
}

void AudioEngine::update(const StateMachine &stateMachine, const Potentiometer &potPitch, const Potentiometer &potTone) {
    StateMachine::State currentState = stateMachine.getState();
    size_t bytes_written;

    if (currentState == StateMachine::MUTE) {
        setMasterVolume(0);
        fillBuffer();
        i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        return;
    }

    const Menu &menu = stateMachine.getMenu();
    int selectedMode = menu.getSelectedMode();
    if (selectedMode == -1) {
        setMasterVolume(0);
        fillBuffer();
        i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        return;
    }
    if (selectedMode >= 0 && selectedMode < 5) {
        currentWaveform = waveforms[selectedMode];
    }
    setFrequency(map(potPitch.getValue(), 0, 4095, 20, 20000));
    //setAmplitude(potTone.getValue() / 4095.0);
    fillBuffer();
    i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
}

void AudioEngine::setWaveform(WaveformGenerator* waveform) {
    currentWaveform = waveform;
}

void AudioEngine::setFrequency(float freq) {
    frequency = freq;
    updatePhaseIncrement();
}

void AudioEngine::setAmplitude(float amp) {
    amplitude = amp;
}

void AudioEngine::setMasterVolume(float vol) {
    masterVolume = vol;
}

void AudioEngine::fillBuffer() {
    if (!currentWaveform) {
        memset(audioBuffer, 0, sizeof(audioBuffer));
        return;
    }

    if (audioState == FEEDBACK_TONE) {
        fillFeedbackBuffer();  
        return;
    }

    for (int i = 0; i < BUFFER_SIZE / 2; i++) {  
        float sample = currentWaveform->getSample(phase) * amplitude * masterVolume;
        int16_t sampleValue = (int16_t)(sample * 32767);
        
        audioBuffer[i * 2] = sampleValue;      
        audioBuffer[i * 2 + 1] = sampleValue;  
        
        phase += phaseIncrement;
        if (phase >= 2*PI) {
            phase -= 2*PI;
        }
    }
}

void AudioEngine::updatePhaseIncrement() {
    phaseIncrement = TWO_PI * frequency / SAMPLE_RATE;
}

void AudioEngine::playFeedbackTone(float frequency, int durationMs) {
    audioState = FEEDBACK_TONE;
    feedbackFrequency = frequency;
    feedbackSamplesRemaining = (durationMs / 1000.0) * SAMPLE_RATE;
    setFrequency(frequency);
}

void AudioEngine::fillFeedbackBuffer() {
    static float feedbackPhase = 0;  
    
    for (int i = 0; i < BUFFER_SIZE / 2; i++) {
        if (feedbackSamplesRemaining <= 0) {
            audioState = NORMAL_PLAYBACK;
            feedbackPhase = 0;  
            audioBuffer[i * 2] = 0;
            audioBuffer[i * 2 + 1] = 0;
            continue;
        }

        float sample = sin(feedbackPhase) * 0.3;
        int16_t sampleValue = (int16_t)(sample * 32767);

        audioBuffer[i * 2] = sampleValue;
        audioBuffer[i * 2 + 1] = sampleValue;

        feedbackPhase += 2 * PI * feedbackFrequency / SAMPLE_RATE;
        if (feedbackPhase >= 2*PI) {
            feedbackPhase -= 2*PI;
        }

        feedbackSamplesRemaining--;
    }
}
