#include "StateMachine.h"
#include <Arduino.h>

StateMachine::StateMachine() 
    : currentState(MUTE), lastInteractionTime(0) {
    lastInteractionTime = millis();
}

void StateMachine::update() {
    if (currentState == MENU && 
        menu.getSelectedMode() >= 0 &&  
        (millis() - lastInteractionTime > MENU_TIMEOUT)) {
        currentState = PLAYING;
    }
}

void StateMachine::onEncoderMoved(int direction) {
    currentState = MENU;

    if (direction > 0) {
        menu.nextItem();
    } else if (direction < 0) {
        menu.previousItem();
    }

    resetTimeout();
}

void StateMachine::onButtonShortPress() {
    if (currentState == MENU) {
        menu.selectCurrentItem();
        currentState = PLAYING;
        resetTimeout();
    }
}

void StateMachine::onButtonLongPress() {
    if (currentState == MUTE) {
        currentState = PLAYING;
    } else {
        currentState = MUTE;
    }
}

Menu& StateMachine::getMenu() {
    return menu;
}

const Menu& StateMachine::getMenu() const {
    return menu;
}

void StateMachine::resetTimeout() {
    lastInteractionTime = millis();
}

StateMachine::State StateMachine::getState() const {
    return currentState;
}