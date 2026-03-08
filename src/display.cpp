#include "display.h"
#include "config.h"

Display::Display(int width, int height, TwoWire* wire) {
    _display = new Adafruit_SSD1306(width, height, wire, -1);
    _initialized = false;
}

bool Display::begin() {
    if (!_display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        return false;
    }
    
    _u8g2font.begin(*_display);
    _initialized = true;
    clear();
    return true;
}

void Display::clear() {
    if (!_initialized) return;
    _display->clearDisplay();
}

// ============ ЗАГОЛОВОК БОЛЬШЕ НЕ НУЖЕН ============
void Display::printTitle(const char* text) {
    // Пустая функция — ничего не делает
}

void Display::printSensorData(const char* name, float value, const char* unit, int line) {
    if (!_initialized) return;
    
    // line = 1,2,3,4 → Y = line * 12
    // line 1: Y=12, line 2: Y=24, line 3: Y=36, line 4: Y=48
    int y = line * 12;
    
    _u8g2font.setFont(u8g2_font_6x13_t_cyrillic);
    _u8g2font.setForegroundColor(SSD1306_WHITE);
    _u8g2font.setCursor(0, y);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s %.1f%s", name, value, unit);
    _u8g2font.print(buffer);
}

void Display::printStatus(const char* text, int line) {
    if (!_initialized) return;
    
    int y = line * 12;
    
    _u8g2font.setFont(u8g2_font_6x13_t_cyrillic);
    _u8g2font.setForegroundColor(SSD1306_WHITE);
    _u8g2font.setCursor(0, y);
    _u8g2font.print(text);
}

void Display::update() {
    if (!_initialized) return;
    _display->display();
}