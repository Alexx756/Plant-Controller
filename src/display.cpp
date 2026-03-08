#include "display.h"
#include "config.h"

Display::Display(int width, int height, TwoWire* wire) {
    _display = new Adafruit_SSD1306(width, height, wire, -1);
    _initialized = false;
}

bool Display::begin() {
    if (!_display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) return false;
    _u8g2font.begin(*_display);
    _initialized = true;
    clear();
    return true;
}

void Display::clear() {
    if (_initialized) _display->clearDisplay();
}

void Display::printTitle(const char* text) {
    if (!_initialized) return;
    _u8g2font.setFont(u8g2_font_7x13_t_cyrillic);
    _u8g2font.setForegroundColor(SSD1306_WHITE);
    _u8g2font.setCursor(0, 12);
    _u8g2font.print(text);
    _display->drawLine(0, 20, 127, 20, SSD1306_WHITE);
}

void Display::printSensorData(const char* name, float value, const char* unit, int line) {
    if (!_initialized) return;
    int y = 24 + line * 12;
    _u8g2font.setFont(u8g2_font_6x13_t_cyrillic);
    _u8g2font.setForegroundColor(SSD1306_WHITE);
    _u8g2font.setCursor(0, y);
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %.1f%s", name, value, unit);
    _u8g2font.print(buf);
}

void Display::printStatus(const char* text, int line) {
    if (!_initialized) return;
    int y = 24 + line * 12;
    _u8g2font.setFont(u8g2_font_5x8_t_cyrillic);
    _u8g2font.setForegroundColor(SSD1306_WHITE);
    _u8g2font.setCursor(0, y);
    _u8g2font.print(text);
}

void Display::update() {
    if (_initialized) _display->display();
}