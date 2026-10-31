#include "DisplayManager.h"
#include "StateMachine.h"
#include "Menu.h"

//constructor
DisplayManager::DisplayManager()
    : display(&SPI, TFT_CS, TFT_DC, TFT_RST), lastUpdateTime(0) {}


void DisplayManager::begin() {
    Serial.println("TFT init OK");
    display.begin();
    display.setRotation(1); // Landscape orientation

    // DSE=1 is the weakest drive strength (R0 ~ 150 ohm); it tames SPI ringing on the breadboard.
    // Must stay after display.begin(): the library configures these pads during begin().
    // Read-modify-write so only the DSE field changes.
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03 = (IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03 & ~IOMUXC_PAD_DSE(7)) | IOMUXC_PAD_DSE(1); // SCK
    Serial.printf("GPIO_B0_03 (SCK) pad: 0x%08lX\n", (unsigned long)IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03); // TEMP-VERIFY
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02 = (IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02 & ~IOMUXC_PAD_DSE(7)) | IOMUXC_PAD_DSE(1); // MOSI
    Serial.printf("GPIO_B0_02 (MOSI) pad: 0x%08lX\n", (unsigned long)IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02); // TEMP-VERIFY

    display.useFrameBuffer(true);

    display.fillScreen(ILI9488_BLACK);
    display.setTextColor(ILI9488_WHITE);
    drawCenteredText("eduLAB", 8, 2);
    drawCenteredText("v6.0 Teensy", 24, 1);
    display.updateScreen();
    delay(2000);
    display.fillScreen(ILI9488_BLACK);

    renderMuted();
}

void DisplayManager::update(const StateMachine& stateMachine, int frequency) {
    if(millis() - lastUpdateTime < UPDATE_INTERVAL) {
        return;
    }
    lastUpdateTime = millis();

    display.fillScreen(ILI9488_BLACK);

    StateMachine::State currentState = stateMachine.getState();

    if (currentState == StateMachine::MUTE) {
        renderMuted();
    }
    else if (currentState == StateMachine::MENU) {
        renderMenu(stateMachine.getMenu());
    }
    else { // PLAYING
        renderPlaying(stateMachine.getMenu().getSelectedMode(), frequency);
    }
    display.updateScreen();
}

void DisplayManager::renderMuted() {
    display.fillRect(sx(0), sy(0), OLED_W * S, OLED_H * S, ILI9488_WHITE);
    display.setTextColor(ILI9488_BLACK);
    drawCenteredText("MUTE", 9, 2);
    display.setTextColor(ILI9488_WHITE);
}

void DisplayManager::renderMenu(const Menu& menu) {
    drawCenteredText("- SELECT MODE -", 0, 1);
    drawCenteredText(menu.getCurrentItem(), 12, 2);
    display.fillTriangle(sx(4), sy(20), sx(10), sy(14), sx(10), sy(26), ILI9488_WHITE);
    display.fillTriangle(sx(124), sy(20), sx(118), sy(14), sx(118), sy(26), ILI9488_WHITE);

    int startDots = 64 - (menu.getItemCount() * 10) / 2;
    for(int i = 0; i < menu.getItemCount(); i++) {
        if (i == menu.getCurrentIndex()) {
            display.fillRect(sx(startDots + (i * 10)), sy(30), 6 * S, 2 * S, ILI9488_WHITE);
        } else {
            display.drawPixel(sx(startDots + (i * 10) + 2), sy(30), ILI9488_WHITE);
        }
    }
}

void DisplayManager::renderPlaying(int selectedMode, int frequency) {
    display.setTextSize(1 * S);
    display.setCursor(sx(0), sy(0));


    const char* modeNames[] = {"SIN", "TRIANGLE", "SQUARE", "SAW", "NOISE"};
    if (selectedMode >= 0 && selectedMode < 5 ) {
        display.print(modeNames[selectedMode]);
    }

    String freqStr = String(frequency) + " Hz";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(OX + OLED_W * S - w, sy(0));
    display.print(freqStr);

    drawWaveIcon(selectedMode, 2, 14);

    display.drawRect(sx(30), sy(14), 90 * S, 14 * S, ILI9488_WHITE);

    int maxFreq = (selectedMode == Menu::NOISE) ? 5000 : 20000;
    int barW = map(frequency, 350, maxFreq, 0, 86);
    if (barW < 0) barW = 0;
    if (barW > 86) barW = 86;

    display.fillRect(sx(32), sy(16), barW * S, 10 * S, ILI9488_WHITE);
}

void DisplayManager::drawCenteredText(const String& text, int y, int size) {
    display.setTextSize(size * S);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int x = OX + (OLED_W * S - w) / 2;
    display.setCursor(x, sy(y));
    display.print(text);
}

void DisplayManager::drawWaveIcon(int mode, int x, int y) {
    uint16_t color = ILI9488_WHITE;

    switch(mode) {
        case Menu::SINE:
            for(int i=0; i<19; i++) {
                int y1 = y + 6 + (int)(5.0 * sin((i / 19.0) * 6.28));
                int y2 = y + 6 + (int)(5.0 * sin(((i+1) / 19.0) * 6.28));
                display.drawLine(sx(x+i), sy(y1), sx(x+i+1), sy(y2), color);
            }
            break;
        case Menu::SQUARE:
            display.drawLine(sx(x), sy(y+10), sx(x+5), sy(y+10), color);
            display.drawLine(sx(x+5), sy(y+10), sx(x+5), sy(y+2), color);
            display.drawLine(sx(x+5), sy(y+2), sx(x+15), sy(y+2), color);
            display.drawLine(sx(x+15), sy(y+2), sx(x+15), sy(y+10), color);
            display.drawLine(sx(x+15), sy(y+10), sx(x+20), sy(y+10), color);
            break;

        case Menu::SAW:
            display.drawLine(sx(x), sy(y+10), sx(x+10), sy(y+2), color);
            display.drawLine(sx(x+10), sy(y+2), sx(x+10), sy(y+10), color);
            display.drawLine(sx(x+10), sy(y+10), sx(x+20), sy(y+2), color);
            display.drawLine(sx(x+20), sy(y+2), sx(x+20), sy(y+10), color);
            break;

        case Menu::TRIANGLE:
            display.drawLine(sx(x), sy(y+10), sx(x+5), sy(y+2), color);
            display.drawLine(sx(x+5), sy(y+2), sx(x+10), sy(y+10), color);
            display.drawLine(sx(x+10), sy(y+10), sx(x+15), sy(y+2), color);
            display.drawLine(sx(x+15), sy(y+2), sx(x+20), sy(y+10), color);
            break;

        case Menu::NOISE:
            for(int i=0; i<20; i+=2) {
                int h = (i % 5) * 2 + 2;
                int offset = (i % 3 == 0) ? 4 : 8;
                display.drawLine(sx(x+i), sy(y+offset), sx(x+i), sy(y+offset+h), color);
            }
            break;
    }
}
