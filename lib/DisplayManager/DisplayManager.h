#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include "ILI9488_t3.h" // For Teensy display

class StateMachine;
class Menu;

class DisplayManager {
private:
    static const int SCREEN_WIDTH = 480;
    static const int SCREEN_HEIGHT = 320;
    static const int TFT_RST = 8;
    static const int TFT_DC = 9;
    static const int TFT_CS = 10;
    ILI9488_t3 display;

    // Scale the 128x32 OLED layout up onto the TFT, centered
    static const int S = 3;
    static const int OLED_W = 128;
    static const int OLED_H = 32;
    static const int OX = (SCREEN_WIDTH - OLED_W * S) / 2;   // 48
    static const int OY = (SCREEN_HEIGHT - OLED_H * S) / 2;  // 112
    int sx(int x) const { return OX + x * S; }
    int sy(int y) const { return OY + y * S; }
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 50;

public:
    DisplayManager();
    void begin();
    void update(const StateMachine& stateMachine, int frequency);
    
private:
    void renderMuted();
    void renderMenu(const Menu& menu);
    
    void renderPlaying(int selectedMode, int frequency);
    
    void drawCenteredText(const String& text, int y, int size);
    void drawWaveIcon(int mode, int x, int y);
};

#endif