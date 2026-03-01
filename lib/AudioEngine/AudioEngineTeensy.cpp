#ifdef TEENSY_BUILD

#include "AudioEngine.h"
#include "Voice.h"
#include "Waveforms/Waveforms.h"
#include <Arduino.h>
#include <imxrt.h>
#include <arm_math.h>
#include <DMAChannel.h>


static DMAChannel dma;

DMAMEM static int32_t buffer_A[256] __attribute__((aligned(32)));;
DMAMEM static int32_t buffer_B[256] __attribute__((aligned(32)));;
static volatile bool dma_playing_A = true;
static int32_t* _fillTarget = nullptr;
AudioEngine* AudioEngine::_instance = nullptr;

const static int AUDIO_SAMPLE_RATE_EXACT = 44117.64706f; // 44.1kHz * 256 samples per buffer


AudioEngine::AudioEngine(int bck, int lrck, int din)
    : audioState(NORMAL_PLAYBACK), feedbackSamplesRemaining(0), feedbackFrequency(0),
      I2S_BCK_PIN(bck), I2S_LRCK_PIN(lrck), I2S_DIN_PIN(din) {
}


void AudioEngine::dmaISR() {
    dma.clearInterrupt();
    dma.clearComplete(); // <--- CRITICAL: Clear the DMA DONE flag
    
    _instance->isrCount++; 
    if (!_instance) return;
    
    if (dma_playing_A) {
        _fillTarget = buffer_A;  
        dma.sourceBuffer(buffer_B, sizeof(buffer_B));
    } else {
        _fillTarget = buffer_B;  
        dma.sourceBuffer(buffer_A, sizeof(buffer_A));
    }
    dma_playing_A = !dma_playing_A;
    
    dma.enable(); // <--- CRITICAL: Re-enable DMA channel for the next block
    
    _instance->fillBuffer();
    arm_dcache_flush_delete(_fillTarget, sizeof(buffer_A)); // Keep size correct (now 1024 bytes)
}


void set_audioClock(int nfact, int32_t nmult, uint32_t ndiv, bool force = false) { 
	if (!force && (CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_ENABLE)) return;

	CCM_ANALOG_PLL_AUDIO = CCM_ANALOG_PLL_AUDIO_BYPASS | CCM_ANALOG_PLL_AUDIO_ENABLE
			     | CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2) // page 1105
			     | CCM_ANALOG_PLL_AUDIO_DIV_SELECT(nfact);

	CCM_ANALOG_PLL_AUDIO_NUM   = nmult & CCM_ANALOG_PLL_AUDIO_NUM_MASK;
	CCM_ANALOG_PLL_AUDIO_DENOM = ndiv & CCM_ANALOG_PLL_AUDIO_DENOM_MASK;
	
	CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_POWERDOWN;//Switch on PLL
	while (!(CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK)) {}; //Wait for pll-lock
	
	const int div_post_pll = 1; // other values: 2,4
	CCM_ANALOG_MISC2 &= ~(CCM_ANALOG_MISC2_DIV_MSB | CCM_ANALOG_MISC2_DIV_LSB);
	if(div_post_pll>1) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_LSB;
	if(div_post_pll>3) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_MSB;
	
	CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_BYPASS;//Disable Bypass
}


void AudioEngine::begin() {
	_instance = this; // Set the static instance pointer for ISR access
	Serial.println("Step 1: CCM clock");
	Serial.printf("I2S_bckPin: %d, I2S_lrckPin: %d, I2S_dinPin: %d\n", I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DIN_PIN);
	
    //Step 1: Enable SAI1 clock
    CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

	
	waveforms[0] = new SineWave();
	Serial.printf("waveforms[0] ptr: %p\n", waveforms[0]);
	if (waveforms[0]) {
		float test = waveforms[0]->getSample(0.0f);
		Serial.printf("getSample test: %f\n", test);
	}
	voices[0].setWaveform(waveforms[0]);
	Serial.printf("voice waveform ptr: %p\n", voices[0].getWaveform());
    waveforms[1] = new TriangleWave();
    waveforms[2] = new SquareWave();
    waveforms[3] = new SawWave();
    waveforms[4] = new NoiseWave();

	//Voice initialization for Teensy (mono)
	voices[0].setAmplitude(1.0f);
	voices[0].setFrequency(440.0f);
	voices[0].noteOn(440.0f, 0.1f);


	Serial.println("Step 3: PLL");
    // Step 3: PLL configuration
    int fs = AUDIO_SAMPLE_RATE_EXACT;
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);

	Serial.println("Step 4: SAI registers");

    // Step 4: SAI1 registers
    CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	// Select MCLK
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1
		& ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
		| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));


	// Step 5:Pin mux
	*(portConfigRegister(23)) = 3; // MCLK - hardcoded, SAI1 only on pin 23
	*(portConfigRegister(I2S_BCK_PIN)) = 3; // ALT3 for SAI1
	*(portConfigRegister(I2S_LRCK_PIN)) = 3; // ALT3 for SAI1
	*(portConfigRegister(I2S_DIN_PIN)) = 3; // ALT3 for SAI1

	int rsync = 0;
	int tsync = 1;

	I2S1_TMR = 0;
	//I2S1_TCSR = (1<<25); //Reset
	I2S1_TCR1 = I2S_TCR1_RFW(1);
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP // sync=0; tx is async;
		    | (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1));
	I2S1_TCR3 = I2S_TCR3_TCE;
	I2S1_TCR4 = I2S_TCR4_FRSZ((2-1)) | I2S_TCR4_SYWD((32-1)) | I2S_TCR4_MF
		    | I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
	I2S1_TCR5 = I2S_TCR5_WNW((32-1)) | I2S_TCR5_W0W((32-1)) | I2S_TCR5_FBT((32-1));

	I2S1_RMR = 0;
	//I2S1_RCSR = (1<<25); //Reset
	I2S1_RCR1 = I2S_RCR1_RFW(1);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP  // sync=0; rx is async;
		    | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
	I2S1_RCR3 = I2S_RCR3_RCE;
	I2S1_RCR4 = I2S_RCR4_FRSZ((2-1)) | I2S_RCR4_SYWD((32-1)) | I2S_RCR4_MF
		    | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S1_RCR5 = I2S_RCR5_WNW((32-1)) | I2S_RCR5_W0W((32-1)) | I2S_RCR5_FBT((32-1));


	Serial.println("Step 5: DMA setup");
    // Step 5: TODO - DMA setup

	dma.sourceBuffer(buffer_A, sizeof(buffer_A));
	dma.destination(I2S1_TDR0);
	dma.transferSize(4);
	dma.transferCount(256);
	dma.interruptAtCompletion();
	dma.attachInterrupt(AudioEngine::dmaISR);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);

	// Start with buffer A, fill it, and flush to ensure it's in RAM before DMA reads it
	_fillTarget = buffer_B; // Fill the non-playing buffer first
	fillBuffer();
	arm_dcache_flush_delete(buffer_B, sizeof(buffer_B));
	_fillTarget = buffer_A;
	fillBuffer();
	arm_dcache_flush_delete(buffer_A, sizeof(buffer_A));	

	dma.enable();

	Serial.println("Step 6: Enable SAI");
	//step 6: Enable SAI
	I2S1_TCSR |= I2S_TCSR_FRDE; // Enable FIFO request when empty 

	I2S1_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // Enable transmitter and bit clock
	I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE; // Enable receiver and bit clock

	Serial.println("Audio Engine Initialized!");
}

void AudioEngine::fillBuffer() {
    bool anyActive = false;
    for (int i=0; i < 256 / 2; i++) {
        float mixedSample = 0.0f;
        for(Voice &voice : voices) {
            if (voice.getIsActive()  && voice.getWaveform() != nullptr) {
                anyActive = true;
                mixedSample += voice.getNextSample();
            }
        }
        mixedSample *= masterVolume;
		//mixedSample /= sizeof(voices) / sizeof(Voice);

        // Convert to 16-bit PCM, then shift to the upper 16 bits of the 32-bit word
        int32_t sampleValue = (int32_t)(mixedSample * 32767.0f);
        
        _fillTarget[i * 2]     = sampleValue << 16; // Left channel
        _fillTarget[i * 2 + 1] = sampleValue << 16; // Right channel
    }

    if (audioState == FEEDBACK_TONE) {
        fillFeedbackBuffer();  
        return;
    }
    
    if (!anyActive) {
        memset(_fillTarget, 0, sizeof(buffer_A)); // Silence
        return;
    }
}

void AudioEngine::noteOn(int voiceIndex, float freq, float amp) {
    voices[voiceIndex].noteOn(freq, amp);
}

void AudioEngine::setMasterVolume(float vol) {
    masterVolume = vol;
}

void AudioEngine::fillFeedbackBuffer() {
    // TODO: implement for Teensy in Phase 3
}


#endif