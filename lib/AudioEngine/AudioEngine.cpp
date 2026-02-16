#include "AudioEngine.h"
#include "StateMachine.h"
#include "Potentiometer.h"
#include "Voice.h"
#include <Arduino.h>
#include "driver/i2s.h"
#include "Waveforms/Waveforms.h"
#include "../../include/Utils.h"
#include "../../include/Consts.h"



AudioEngine::AudioEngine(int bck, int lrck, int din)
    : audioState(NORMAL_PLAYBACK), feedbackSamplesRemaining(0), feedbackFrequency(0),
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

    for (int i = 0; i < 4; i++) {
        voices[i] = Voice(waveforms[0], 0.0f, 0.0f);
    }

    //test to see if polyphony works
    float freq = 440.0f; // A4
    noteOn(0, freq, 0.5f); 
    noteOn(1, 1.25 * freq, 0.5f); 
    noteOn(2, 1.5 * freq, 0.5f); 

    Serial.println("I2S Initialized!");
}

void AudioEngine::update(const StateMachine &stateMachine, const Potentiometer &potPitch, const Potentiometer &potTone) {
    size_t bytes_written;
    StateMachine::State currentState = stateMachine.getState();

    if (audioState == FEEDBACK_TONE) {
        fillFeedbackBuffer();
        i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        return;
    }

    if (currentState == StateMachine::MUTE) {
        memset(audioBuffer, 0, sizeof(audioBuffer));
        i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        return;
    }

    int selectedMode = stateMachine.getMenu().getSelectedMode();

    if (selectedMode == -1) {
        memset(audioBuffer, 0, sizeof(audioBuffer));
        i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        return;
    }

    if (selectedMode >= 0 && selectedMode < 5) {
        //setWaveform(selectedMode, waveforms[selectedMode]);
        //test to see if polyphony works with different frequencies
        setWaveform(0, waveforms[selectedMode]);
        setWaveform(1, waveforms[selectedMode]);
        setWaveform(2, waveforms[selectedMode]);
    } else {
        memset(audioBuffer, 0, sizeof(audioBuffer));
        i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        return;
    }

    int maxFreq = (selectedMode == 4) ? 5000 : 20000; // 4 is NOISE
    
    
    //test to see if polyphony works with different frequencies
    float baseFreq = mapLogarithmicAsymmetric(potPitch.getValue(), 20.0f, maxFreq);
    setFrequency(0, baseFreq);
    setFrequency(1, 1.25 * baseFreq);
    setFrequency(2, 1.5 * baseFreq);
    

    float vol = potTone.getValue() / 4095.0f;
    setMasterVolume(vol * 0.1f); 

    fillBuffer();
    i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
}

void AudioEngine::setWaveform(int voiceIndex, WaveformGenerator* waveform) {
    voices[voiceIndex].setWaveform(waveform);
}

void AudioEngine::setFrequency(int voiceIndex, float freq) {
    voices[voiceIndex].setFrequency(freq);
}

void AudioEngine::setAmplitude(int voiceIndex, float amp) {
    voices[voiceIndex].setAmplitude(amp);
}

void AudioEngine::noteOn(int voiceIndex, float freq, float amp) {
    voices[voiceIndex].noteOn(freq, amp);
}

void AudioEngine::noteOff(int voiceIndex) {
    voices[voiceIndex].noteOff();
}

void AudioEngine::setMasterVolume(float vol) {
    masterVolume = vol;
}

void AudioEngine::fillBuffer() {
    bool anyActive = false;
    for (int i=0; i < BUFFER_SIZE / 2; i++) {
        float mixedSample = 0.0f;
        for(Voice &voice : voices) {
            if (voice.getIsActive()) {
                anyActive = true;
                float sample = voice.getNextSample();
                mixedSample += sample;
            }
        }
        mixedSample *= masterVolume;
        mixedSample /= sizeof(voices) / sizeof(Voice);

        int16_t sampleValue = (int16_t)(mixedSample * 32767);

        audioBuffer[i * 2] = sampleValue;      
        audioBuffer[i * 2 + 1] = sampleValue;
    }

    if (!anyActive) {
        memset(audioBuffer, 0, sizeof(audioBuffer));
        return;
    }

    if (audioState == FEEDBACK_TONE) {
        fillFeedbackBuffer();  
        return;
    }
}

void AudioEngine::playFeedbackTone(float frequency, int durationMs) {
    audioState = FEEDBACK_TONE;
    feedbackFrequency = frequency;
    feedbackSamplesRemaining = (durationMs / 1000.0) * SAMPLE_RATE;
    for (int i = 0; i < sizeof(voices) / sizeof(Voice); i++) {
        setFrequency(i, frequency);
    }
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

        // OLD: fixed 0.3 amplitude
        // float sample = sin(feedbackPhase) * 0.3;
        
        // NEW: respect masterVolume, but cap at safe level
        float feedbackAmplitude = min(masterVolume * 0.5f, 0.15f);  // Max 15% even if volume is high
        float sample = sin(feedbackPhase) * feedbackAmplitude;
        
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
