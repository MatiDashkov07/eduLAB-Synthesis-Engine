#include "StateMachine.h"
#include <Arduino.h>

StateMachine::StateMachine() 
    : currentState(MUTE), lastInteractionTime(0) {
    lastInteractionTime = millis();
}

void StateMachine::update() {
    if (currentState == MENU && (millis() - lastInteractionTime > MENU_TIMEOUT)) {
        currentState = PLAYING;
    }
}

void StateMachine::onEncoderMoved(int direction) {
    // ✅ MUTED: ignore encoder
    if (currentState == MUTE) {
        return;
    }

    currentState = MENU;

    if (direction > 0) {
        menu.nextItem();
    } else if (direction < 0) {
        menu.previousItem();
    }

    resetTimeout();
}

void StateMachine::onButtonShortPress() {
    // ✅ תיקון: רק ב-MENU מגיב
    if (currentState == MENU) {
        menu.selectCurrentItem();
        currentState = PLAYING;
        resetTimeout();
    }
    // MUTE/PLAYING: לא עושים כלום
}

void StateMachine::onButtonLongPress() {
    // ✅ Toggle MUTE ↔ PLAYING
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