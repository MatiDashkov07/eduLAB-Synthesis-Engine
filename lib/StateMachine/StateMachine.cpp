#include "StateMachine.h"
#include <Arduino.h>

StateMachine::StateMachine() 
    : currentState(MENU), lastInteractionTime(0) {
    lastInteractionTime = millis();
}

void StateMachine::update() {
    // Check for timeout to return to PLAYING state
    if (currentState == MENU && (millis() - lastInteractionTime > MENU_TIMEOUT)) {
        currentState = PLAYING;
    }
}

void StateMachine::onEncoderMoved(int direction) {
    //MUTED: ignore encoder (must-long press to unmute)
    if (currentState == MUTE) {
        return;
    }

    currentState = MENU;

    //Navigate menu
    if (direction > 0) {
        menu.nextItem();
    } else if (direction < 0) {
        menu.previousItem();
    }

    resetTimeout();
}

void StateMachine::onButtonShortPress() {
    //MUTED: ignore short press (must-long press to unmute)
    if (currentState == MENU) {
        menu.selectCurrentItem();
        currentState = PLAYING;
    }

    //PLAYING/MUTE: do nothing
}

void StateMachine::onButtonLongPress() {
    //Toggle MUTE state
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