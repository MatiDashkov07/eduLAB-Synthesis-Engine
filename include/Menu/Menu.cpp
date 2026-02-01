#include "Menu.h"
#include <Arduino.h>

    void Menu::nextItem() {
        currentIndex = (currentIndex + 1) % 4;
    }

    void Menu::previousItem() {
        currentIndex = (currentIndex - 1 + 4) % 4;
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
        if (index >= 0 && index < 4) {
            return items[index];
        }
        return "";
    }

    int Menu::getItemCount() const {
        return 4;
    }
