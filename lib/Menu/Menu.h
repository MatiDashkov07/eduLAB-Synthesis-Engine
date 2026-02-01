#ifndef MENU_H
#define MENU_H

class Menu {
public:
    enum WaveMode { SQUARE = 0, SAW = 1, TRIANGLE = 2, NOISE = 3 };
    
private:
    const char* items[4] = {"SQUARE", "SAW", "TRIANGLE", "NOISE"};
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