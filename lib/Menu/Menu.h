#ifndef MENU_H
#define MENU_H

class Menu {
public:
    enum WaveformMode {
    SINE = 0,
    TRIANGLE = 1,
    SQUARE = 2,
    SAW = 3,
    NOISE = 4
};
    
private:
    const char* items[5] = {"SINE", "TRIANGLE", "SQUARE", "SAW", "NOISE"};
    int currentIndex;
    int selectedMode;

public:
    Menu() : currentIndex(0), selectedMode(-1) {}

    void nextItem();
    void previousItem();
    void selectCurrentItem();
    
    int getCurrentIndex() const;
    int getSelectedMode() const;
    const char* getCurrentItem() const;
    const char* getItem(int index) const;
    int getItemCount() const;
};

#endif