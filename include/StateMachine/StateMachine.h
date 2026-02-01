#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "../Menu/Menu.h"
#include <Arduino.h>

class StateMachine 
{
    public:
        enum State {
            PLAYING,
            MENU,
            MUTE
        };

    private:
        State currentState;
        Menu menu;
        unsigned long lastInteractionTime;
        static const unsigned long MENU_TIMEOUT = 10000; // 10 seconds


    public:
        StateMachine();

        void update();

        void onEncoderMoved(int direction);
        void onButtonShortPress();
        void onButtonLongPress();

        State getState() const;
        Menu &getMenu();
        const Menu &getMenu() const; //const version
        
    private:    
        void resetTimeout();
    };

#endif