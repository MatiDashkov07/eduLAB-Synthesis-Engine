#include "Menu.h"
#include <Arduino.h>

    void Menu::nextItem() {
        currentIndex = (currentIndex + 1) % 5;
    }

    void Menu::previousItem() {
        currentIndex = (currentIndex - 1 + 5) % 5;
    }

    void Menu::selectCurrentItem() {
        selectedMode = currentIndex;
    }

    int Menu::getCurrentIndex() const {
        return currentIndex;
    }

    int Menu::getSelectedMode() const {
        return selectedMode;
    }

    const char* Menu::getCurrentItem() const {
        return items[currentIndex];
    }

    const char* Menu::getItem(int index) const {
        if (index >= 0 && index < 5) {
            return items[index];
        }
        return "";
    }

    int Menu::getItemCount() const {
        return 5;
    }
