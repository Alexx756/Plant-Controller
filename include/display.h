#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>

class Display {
private:
    Adafruit_SSD1306* _display;
    U8G2_FOR_ADAFRUIT_GFX _u8g2font;
    bool _initialized;
public:
    Display(int width, int height, TwoWire* wire);
    bool begin();
    void clear();
    void printTitle(const char* text);
    void printSensorData(const char* name, float value, const char* unit, int line);
    void printStatus(const char* text, int line);
    void update();
    U8G2_FOR_ADAFRUIT_GFX* getU8g2() { return &_u8g2font; }
    Adafruit_SSD1306* getDisplay() { return _display; }
};

#endif