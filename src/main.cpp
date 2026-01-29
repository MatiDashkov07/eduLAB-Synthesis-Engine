#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Potentiometer/Potentiometer.h"

// ==========================================
// 1. HARDWARE CONFIGURATION
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 4
#define I2C_SCL 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Mapping
const int SW_PIN        = 15;
const int POT_PIN_PITCH = 1;
const int POT_PIN_TONE  = 2;
const int BUZZER_PIN    = 16;
const int PIN_DT        = 7;
const int PIN_CLK       = 6;

// Audio Constants
const int LEDC_CHANNEL    = 0;
const int LEDC_RESOLUTION = 8;
const int BASE_FREQ       = 5000;
const int MIN_FREQ_SAFE   = 350; // הוחזר ל-350Hz ליציבות



// יצירת אובייקטים עם הפרמטרים המעודכנים
Potentiometer potPitch(POT_PIN_PITCH, 0.15, 40); 
Potentiometer potTone(POT_PIN_TONE, 0.15, 40);

// ==========================================
// 3. STATE MANAGEMENT
// ==========================================
enum UIState { STATE_PLAYING, STATE_MENU };
UIState currentUIState = STATE_MENU;

const char* menuItems[] = {"SQUARE", "SAW", "TRIANGLE", "NOISE"};
int menuIndex = 0;          
int selectedMode = -1; 

bool buttonActive = false;       
bool longPressHandled = false;   
unsigned long pressStartTime = 0; 
const unsigned long LONG_PRESS_TIME = 800; 
bool isMuted = true; 

volatile int virtualPosition = 0; 
int lastPosition = 0;
volatile unsigned long lastInterruptTime = 0;
unsigned long lastInteractionTime = 0;
const unsigned long MENU_TIMEOUT = 10000;
unsigned long lastDisplayUpdate = 0;
unsigned long lastNoiseUpdate = 0; 

int lastAppliedFreq = 0;
int lastAppliedDuty = -1;
bool forceUpdate = false;

// ==========================================
// 4. ISR & HELPER FUNCTIONS
// ==========================================
void IRAM_ATTR updateEncoder() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 10) {
    if (digitalRead(PIN_CLK) != digitalRead(PIN_DT)) {
      virtualPosition++;
    } else {
      virtualPosition--;
    }
    lastInterruptTime = interruptTime;
  }
}

void playFeedbackTone(int frequency, int duration) {
    ledcChangeFrequency(LEDC_CHANNEL, frequency, LEDC_RESOLUTION);
    ledcWrite(LEDC_CHANNEL, 128); 
    delay(duration);
    ledcWrite(LEDC_CHANNEL, 0); 
    forceUpdate = true; 
    lastAppliedFreq = 0; 
}

// ==========================================
// 5. GRAPHICS
// ==========================================
void drawCenteredText(String text, int y, int size) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

void drawWaveIcon(int mode, int x, int y) {
  uint16_t color = SSD1306_WHITE;
  switch(mode) {
    case 0: // Square
      display.drawLine(x, y+10, x+5, y+10, color); display.drawLine(x+5, y+10, x+5, y+2, color);
      display.drawLine(x+5, y+2, x+15, y+2, color); display.drawLine(x+15, y+2, x+15, y+10, color);
      display.drawLine(x+15, y+10, x+20, y+10, color); break;
    case 1: // Saw
      display.drawLine(x, y+10, x+10, y+2, color); display.drawLine(x+10, y+2, x+10, y+10, color);
      display.drawLine(x+10, y+10, x+20, y+2, color); display.drawLine(x+20, y+2, x+20, y+10, color); break;
    case 2: // Triangle
      display.drawLine(x, y+10, x+5, y+2, color); display.drawLine(x+5, y+2, x+10, y+10, color);
      display.drawLine(x+10, y+10, x+15, y+2, color); display.drawLine(x+15, y+2, x+20, y+10, color); break;
    case 3: // Noise
      for(int i=0; i<20; i+=2) {
         int h = (i % 5) * 2 + 2; 
         int offset = (i % 3 == 0) ? 4 : 8;
         display.drawLine(x+i, y+offset, x+i, y+offset+h, color);
      } break;
  }
}

void updateDisplay() {
  display.clearDisplay();
  if (isMuted) {
    display.fillRect(0, 0, 128, 32, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    drawCenteredText("MUTE", 9, 2);
    display.setTextColor(SSD1306_WHITE);
  } 
  else if (currentUIState == STATE_MENU || selectedMode == -1) { 
    drawCenteredText("- SELECT MODE -", 0, 1);
    drawCenteredText(menuItems[menuIndex], 12, 2);
    display.fillTriangle(4, 20, 10, 14, 10, 26, SSD1306_WHITE);
    display.fillTriangle(124, 20, 118, 14, 118, 26, SSD1306_WHITE);
    int startDots = 64 - (4*10)/2;
    for(int i=0; i<4; i++) {
       if (i == menuIndex) display.fillRect(startDots + (i*10), 30, 6, 2, SSD1306_WHITE);
       else display.drawPixel(startDots + (i*10) + 2, 30, SSD1306_WHITE);
    }
  } 
  else { // PLAYING
    display.setTextSize(1); display.setCursor(0, 0); display.print(menuItems[selectedMode]);
    int currentFreq = map(potPitch.getValue(), 0, 4095, MIN_FREQ_SAFE, (selectedMode==3 ? 5000 : 2000));
    String freqStr = String(currentFreq) + " Hz";
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(128 - w, 0); display.print(freqStr);
    drawWaveIcon(selectedMode, 2, 14); 
    display.drawRect(30, 14, 90, 14, SSD1306_WHITE);
    int barW = map(potPitch.getValue(), 0, 4095, 0, 86);
    display.fillRect(32, 16, barW, 10, SSD1306_WHITE);  
  }
  display.display();
}

// ==========================================
// 6. SETUP
// ==========================================
void setup() {
  delay(500); 
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) for(;;);
  
  display.clearDisplay();
  drawCenteredText("SONIC LAB S3", 10, 1);
  display.display();
  delay(1000);

  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT);
  pinMode(PIN_CLK, INPUT);

  ledcSetup(LEDC_CHANNEL, BASE_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), updateEncoder, FALLING);

  potPitch.begin();
  potTone.begin();

  updateDisplay(); 
}

// ==========================================
// 7. MAIN LOOP
// ==========================================
void loop() {
  // --- A. כפתור ---
  int reading = digitalRead(SW_PIN);
  if (reading == LOW && !buttonActive) {
    buttonActive = true; pressStartTime = millis(); longPressHandled = false; delay(10); 
  }
  if (buttonActive && reading == LOW) {
    if ((millis() - pressStartTime > LONG_PRESS_TIME) && !longPressHandled) {
      isMuted = !isMuted; longPressHandled = true;
      playFeedbackTone(500, 100);
      forceUpdate = true; updateDisplay(); 
    }
  }
  if (reading == HIGH && buttonActive) {
    buttonActive = false;
    if (!longPressHandled) {
       selectedMode = menuIndex;
       currentUIState = STATE_PLAYING; 
       playFeedbackTone(2000, 50);
       forceUpdate = true; 
    }
  }

  // --- B. UI Logic & ATOMIC READ ---
  
  // הגנה מפני Race Condition בעת קריאת משתנה ה-Interrupt
  int currentPos;
  noInterrupts(); 
  currentPos = virtualPosition;
  interrupts();

  if (currentPos != lastPosition) {
    lastPosition = currentPos;
    menuIndex = abs(currentPos) % 4;
    currentUIState = STATE_MENU;
    lastInteractionTime = millis(); 
  }
  
  if (currentUIState == STATE_MENU && (millis() - lastInteractionTime > MENU_TIMEOUT)) {
    currentUIState = STATE_PLAYING;
  }

  if (millis() - lastDisplayUpdate > 50) { 
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // --- C. INPUT PROCESSING ---
  bool pitchChanged = potPitch.update();
  bool toneChanged = potTone.update();

  // --- D. AUDIO ENGINE ---
  if (!isMuted && selectedMode != -1) {
      
      // מצב NOISE
      if (selectedMode == 3) {
          if (millis() - lastNoiseUpdate > 5) { 
             int noiseCeiling = map(potPitch.getValue(), 0, 4095, 600, 8000);
             if (noiseCeiling < 600) noiseCeiling = 600;
             
             int noiseFreq = random(200, noiseCeiling);
             int noiseDuty = random(10, 255);

             // תיקון סדר הפעולות: קודם תדר, אחר כך Duty
             ledcChangeFrequency(LEDC_CHANNEL, noiseFreq, LEDC_RESOLUTION);
             ledcWrite(LEDC_CHANNEL, noiseDuty); 
             
             lastNoiseUpdate = millis();
          }
      } 
      // מצבים רגילים
      else {
          if (pitchChanged || toneChanged || forceUpdate) {
              int targetFrequency = map(potPitch.getValue(), 0, 4095, MIN_FREQ_SAFE, 2000);
              int targetDuty = map(potTone.getValue(), 0, 4095, 0, 255); 

              if (targetFrequency != lastAppliedFreq || forceUpdate) {
                  ledcChangeFrequency(LEDC_CHANNEL, targetFrequency, LEDC_RESOLUTION);
                  lastAppliedFreq = targetFrequency;
              }
              if (targetDuty != lastAppliedDuty || forceUpdate) {
                  ledcWrite(LEDC_CHANNEL, targetDuty);
                  lastAppliedDuty = targetDuty;
              }
              forceUpdate = false;
          }
      }
  } else {
      // השתקה
      if (lastAppliedDuty != 0) {
          ledcWrite(LEDC_CHANNEL, 0);
          lastAppliedDuty = 0;
      }
  }
}