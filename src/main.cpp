#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==========================================
// 1. הגדרות חומרה
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 
#define I2C_SDA 18
#define I2C_SCL 19

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int SW_PIN        = 7;    
const int POT_PIN_PITCH = 0;    
const int POT_PIN_TONE  = 1;    
const int BUZZER_PIN    = 4;    
const int PIN_DT        = 5;    
const int PIN_CLK       = 6;    

const int LEDC_CHANNEL    = 0;  
const int LEDC_RESOLUTION = 8;  
const int BASE_FREQ       = 5000; 

// ==========================================
// 2. ניהול מצבים
// ==========================================
enum UIState {
  STATE_PLAYING, 
  STATE_MENU     
};

UIState currentUIState = STATE_MENU; 
unsigned long lastInteractionTime = 0;
const unsigned long MENU_TIMEOUT = 10000; 

const char* menuItems[] = {"SQUARE", "SAW", "TRIANGLE", "NOISE"};
int menuIndex = 0;           
int selectedMode = -1;       

// כפתור
bool buttonActive = false;       
bool longPressHandled = false;   
unsigned long pressStartTime = 0; 
const unsigned long LONG_PRESS_TIME = 800; 

// === שינוי 1: המערכת מתחילה במצב מושתק ===
bool isMuted = true;            

// אנקודר
volatile int virtualPosition = 0; 
int lastPosition = 0;
volatile unsigned long lastInterruptTime = 0;

// DSP
float smoothedPitch = 0; 
const float alpha = 0.1; 
unsigned long lastDisplayUpdate = 0;

// ==========================================
// 3. פסיקות
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

// ==========================================
// 4. גרפיקה
// ==========================================

void drawCenteredText(String text, int y, int size) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

// === שינוי 2: מסך פתיחה מרשים ===
void showSplashScreen() {
  display.clearDisplay();
  
  // אפקט פתיחה: מסגרת נבנית
  display.drawRect(0, 0, 128, 32, SSD1306_WHITE);
  display.display();
  delay(200);
  
  // שם המעבדה
  display.setTextColor(SSD1306_WHITE);
  drawCenteredText("SONIC LAB", 5, 2);
  display.display();
  delay(500);
  
  // גרסה וטעינה
  drawCenteredText("v3.1 Booting...", 22, 1);
  display.display();
  delay(1500); // השהייה כדי שיראו את הלוגו
}

void drawWaveIcon(int mode, int x, int y) {
  uint16_t color = SSD1306_WHITE;
  switch(mode) {
    case 0: // Square
      display.drawLine(x, y+10, x+5, y+10, color);
      display.drawLine(x+5, y+10, x+5, y+2, color);
      display.drawLine(x+5, y+2, x+15, y+2, color);
      display.drawLine(x+15, y+2, x+15, y+10, color);
      display.drawLine(x+15, y+10, x+20, y+10, color);
      break;
    case 1: // Saw
      display.drawLine(x, y+10, x+10, y+2, color);
      display.drawLine(x+10, y+2, x+10, y+10, color);
      display.drawLine(x+10, y+10, x+20, y+2, color);
      display.drawLine(x+20, y+2, x+20, y+10, color);
      break;
    case 2: // Triangle
      display.drawLine(x, y+10, x+5, y+2, color);
      display.drawLine(x+5, y+2, x+10, y+10, color);
      display.drawLine(x+10, y+10, x+15, y+2, color);
      display.drawLine(x+15, y+2, x+20, y+10, color);
      break;
    case 3: // Noise
      for(int i=0; i<20; i+=2) {
         int h = (i % 5) * 2 + 2; 
         int offset = (i % 3 == 0) ? 4 : 8;
         display.drawLine(x+i, y+offset, x+i, y+offset+h, color);
      }
      break;
  }
}

void updateDisplay() {
  display.clearDisplay();

  // 1. MUTE
  if (isMuted) {
    display.fillRect(0, 0, 128, 32, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    drawCenteredText("MUTE", 9, 2);
    display.setTextColor(SSD1306_WHITE);
  } 
  
  // 2. MENU
  else if (currentUIState == STATE_MENU) {
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
  
  // 3. PLAYING
  else {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(menuItems[selectedMode]);
    
    int currentFreq = map((int)smoothedPitch, 0, 4095, 350, (selectedMode==3 ? 5000 : 2000));
    String freqStr = String(currentFreq) + " Hz";
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(128 - w, 0);
    display.print(freqStr);

    drawWaveIcon(selectedMode, 2, 14); 
    display.drawRect(30, 14, 90, 14, SSD1306_WHITE);
    int barW = map((int)smoothedPitch, 0, 4095, 0, 86);
    display.fillRect(32, 16, barW, 10, SSD1306_WHITE);  
  }
  display.display();
}

// ==========================================
// 5. Setup (Boot Sequence)
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) for(;;);
  
  // הרצת מסך הפתיחה
  showSplashScreen();

  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT);
  pinMode(PIN_CLK, INPUT);

  ledcSetup(LEDC_CHANNEL, BASE_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), updateEncoder, FALLING);

  // === שינוי 3: כפיית עדכון מסך ראשוני ===
  // זה מונע את ה"באג" שרואים את המסך הישן לשבריר שנייה
  // כיוון ש-isMuted = true, זה יציג מיד את מסך ה-MUTE
  updateDisplay(); 
  
  Serial.println("--- SYSTEM READY ---");
}

void loop() {
  // A. כפתור
  int reading = digitalRead(SW_PIN);
  if (reading == LOW && !buttonActive) {
    buttonActive = true; pressStartTime = millis(); longPressHandled = false; delay(5); 
  }
  
  if (buttonActive && reading == LOW) {
    if ((millis() - pressStartTime > LONG_PRESS_TIME) && !longPressHandled) {
      isMuted = !isMuted; longPressHandled = true;
      ledcWriteTone(LEDC_CHANNEL, 500); delay(100); ledcWrite(LEDC_CHANNEL, 0); 
      updateDisplay(); // עדכון מיידי בלחיצה
    }
  }
  
  if (reading == HIGH && buttonActive) {
    buttonActive = false;
    if (!longPressHandled) {
       selectedMode = menuIndex;
       currentUIState = STATE_PLAYING; 
       lastInteractionTime = millis();
       ledcWriteTone(LEDC_CHANNEL, 2000); delay(50);
       // אם היינו ב-Mute ויצאנו לנגינה - נבטל את ה-Mute אוטומטית?
       // לשיקולך. כרגע זה משאיר את ה-Mute כפי שהוא עד שתבטל ידנית.
    }
  }

  // B. UI Logic
  if (virtualPosition != lastPosition) {
    lastPosition = virtualPosition;
    menuIndex = abs(virtualPosition) % 4;
    currentUIState = STATE_MENU;
    lastInteractionTime = millis(); 
  }
  
  if (currentUIState == STATE_MENU && selectedMode != -1) {
    if (millis() - lastInteractionTime > MENU_TIMEOUT) {
      currentUIState = STATE_PLAYING;
    }
  }

  if (millis() - lastDisplayUpdate > 33) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // C. סאונד
  if (selectedMode != -1 && !isMuted) {
      int rawPitch = analogRead(POT_PIN_PITCH);
      int rawTone = analogRead(POT_PIN_TONE);
      
      smoothedPitch = (alpha * rawPitch) + ((1.0 - alpha) * smoothedPitch);
      int targetFrequency = map((int)smoothedPitch, 0, 4095, 350, 2000);
      if (targetFrequency < 350) targetFrequency = 350;
      int targetDuty = map(rawTone, 0, 4095, 0, 128); 

      if (selectedMode == 3) { 
         int noiseCeiling = map((int)smoothedPitch, 0, 4095, 600, 5000);
         if (noiseCeiling < 600) noiseCeiling = 600;
         ledcWrite(LEDC_CHANNEL, random(0, 255)); 
         ledcChangeFrequency(LEDC_CHANNEL, random(600, noiseCeiling), LEDC_RESOLUTION);
      } else { 
         ledcChangeFrequency(LEDC_CHANNEL, targetFrequency, LEDC_RESOLUTION);
         ledcWrite(LEDC_CHANNEL, targetDuty);
      }
  } else {
    ledcWrite(LEDC_CHANNEL, 0);
  }
  delay(5);
}