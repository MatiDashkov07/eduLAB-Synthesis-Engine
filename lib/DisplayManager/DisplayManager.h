#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

class StateMachine;
class Menu;

class DisplayManager 
{
    private:
        Adafruit_SSD1306 display;
        unsigned long lastUpdateTime;
        static const unsigned long UPDATE_INTERVAL = 50; // milliseconds

    public:
        DisplayManager();
        void begin();
        void update(const StateMachine& stateMachine, int frequency);
        
    private:
        void renderMuted();
        void renderMenu(const Menu& menu);      
        void renderPlaying(const Menu& menu, int frequency);   
        
        void drawCenteredText(const String& text, int y, int size);
        void drawWaveIcon(int mode, int x, int y);
};

#endif