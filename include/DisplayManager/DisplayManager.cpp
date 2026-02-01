#include "DisplayManager.h"
#include "StateMachine/StateMachine.h" 
#include "Menu/Menu.h"                  

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
    drawCenteredText("SONIC LAB S3", 10, 1);
    display.display();
    delay(1000);
}

void DisplayManager::update(const StateMachine& stateMachine, int frequency) {
    if(millis() - lastUpdateTime < UPDATE_INTERVAL) {
        return; // Not time to update yet
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
        renderPlaying(stateMachine.getMenu(), frequency);
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

void DisplayManager::renderPlaying(const Menu& menu, int frequency) {
    display.setTextSize(1); 
    display.setCursor(0, 0); 
    display.print(menu.getItem(menu.getSelectedMode()));
    
    String freqStr = String(frequency) + " Hz";
    int16_t x1, y1; 
    uint16_t w, h;
    display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(128 - w, 0); 
    display.print(freqStr);
    
    drawWaveIcon(menu.getSelectedMode(), 2, 14); 
    
    display.drawRect(30, 14, 90, 14, SSD1306_WHITE);
    int barW = map(frequency    , 0, 8000, 0, 86); // assuming max frequency is 8000Hz
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

