#include "DisplayManager.h"
#include "StateMachine.h" 
#include "Menu.h"                  

//constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 4
#define I2C_SCL 5

//constructor
DisplayManager::DisplayManager() 
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), lastUpdateTime(0) {
}

void DisplayManager::begin() {
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for (;;);
    }
    
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    drawCenteredText("eduLAB", 8, 2);    
    drawCenteredText("v3.8 OOP", 24, 1);   
    display.display();
    delay(2000);
    display.clearDisplay();
    
    renderMuted();
}

void DisplayManager::update(const StateMachine& stateMachine, int frequency) {
    if(millis() - lastUpdateTime < UPDATE_INTERVAL) {
        return;
    }
    lastUpdateTime = millis();

    display.clearDisplay();

    StateMachine::State currentState = stateMachine.getState();
    
    if (currentState == StateMachine::MUTE) {
        renderMuted();
    } 
    else if (currentState == StateMachine::MENU) {
        renderMenu(stateMachine.getMenu());
    } 
    else { // PLAYING
        renderPlaying(stateMachine.getMenu().getSelectedMode(), frequency);
    }
    display.display();
}

void DisplayManager::renderMuted() {
    display.fillRect(0, 0, 128, 32, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    drawCenteredText("MUTE", 9, 2);
    display.setTextColor(SSD1306_WHITE);
}

void DisplayManager::renderMenu(const Menu& menu) {
    drawCenteredText("- SELECT MODE -", 0, 1);
    drawCenteredText(menu.getCurrentItem(), 12, 2);
    display.fillTriangle(4, 20, 10, 14, 10, 26, SSD1306_WHITE);
    display.fillTriangle(124, 20, 118, 14, 118, 26, SSD1306_WHITE);
    
    int startDots = 64 - (menu.getItemCount() * 10) / 2;
    for(int i = 0; i < menu.getItemCount(); i++) {
        if (i == menu.getCurrentIndex()) {
            display.fillRect(startDots + (i * 10), 30, 6, 2, SSD1306_WHITE);
        } else {
            display.drawPixel(startDots + (i * 10) + 2, 30, SSD1306_WHITE);
        }
    }
}

void DisplayManager::renderPlaying(int selectedMode, int frequency) {
    display.setTextSize(1); 
    display.setCursor(0, 0);
    
    
    const char* modeNames[] = {"SIN", "TRIANGLE", "SQUARE", "SAW", "NOISE"};
    if (selectedMode >= 0 && selectedMode < 5 ) {
        display.print(modeNames[selectedMode]);
    }
    
    String freqStr = String(frequency) + " Hz";
    int16_t x1, y1; 
    uint16_t w, h;
    display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(128 - w, 0); 
    display.print(freqStr);
    
    drawWaveIcon(selectedMode, 2, 14); 
    
    display.drawRect(30, 14, 90, 14, SSD1306_WHITE);
    
    int maxFreq = (selectedMode == Menu::NOISE) ? 5000 : 20000;
    int barW = map(frequency, 350, maxFreq, 0, 86);
    if (barW < 0) barW = 0;
    if (barW > 86) barW = 86;
    
    display.fillRect(32, 16, barW, 10, SSD1306_WHITE);  
}

void DisplayManager::drawCenteredText(const String& text, int y, int size) {
    display.setTextSize(size);
    int16_t x1, y1; 
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - w) / 2;
    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::drawWaveIcon(int mode, int x, int y) {
    uint16_t color = SSD1306_WHITE;
    
    switch(mode) {
        case Menu::SINE:    
            for(int i=0; i<19; i++) {
                int y1 = y + 6 + (int)(5.0 * sin((i / 19.0) * 6.28));
                int y2 = y + 6 + (int)(5.0 * sin(((i+1) / 19.0) * 6.28));
                display.drawLine(x+i, y1, x+i+1, y2, color);
            }
            break;
        case Menu::SQUARE:   
            display.drawLine(x, y+10, x+5, y+10, color); 
            display.drawLine(x+5, y+10, x+5, y+2, color);
            display.drawLine(x+5, y+2, x+15, y+2, color); 
            display.drawLine(x+15, y+2, x+15, y+10, color);
            display.drawLine(x+15, y+10, x+20, y+10, color); 
            break;
            
        case Menu::SAW:     
            display.drawLine(x, y+10, x+10, y+2, color); 
            display.drawLine(x+10, y+2, x+10, y+10, color);
            display.drawLine(x+10, y+10, x+20, y+2, color); 
            display.drawLine(x+20, y+2, x+20, y+10, color); 
            break;
            
        case Menu::TRIANGLE:
            display.drawLine(x, y+10, x+5, y+2, color); 
            display.drawLine(x+5, y+2, x+10, y+10, color);
            display.drawLine(x+10, y+10, x+15, y+2, color); 
            display.drawLine(x+15, y+2, x+20, y+10, color); 
            break;
            
        case Menu::NOISE:
            for(int i=0; i<20; i+=2) {
                int h = (i % 5) * 2 + 2; 
                int offset = (i % 3 == 0) ? 4 : 8;
                display.drawLine(x+i, y+offset, x+i, y+offset+h, color);
            } 
            break;
    }
}