#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "config.h"
#include "relay.h"
#include "schedule.h"  

// Типы экранов
enum ScreenType {
    SCREEN_MAIN,                    // Главный экран с датчиками
    SCREEN_MODE_SELECT,              // Выбор режима (AUTO/MANUAL/SETTINGS)
    SCREEN_MANUAL,                   // Ручное управление реле
    SCREEN_SETTINGS,                  // Меню настроек
    SCREEN_THRESHOLDS,                // Настройка порогов
    SCREEN_STATS,                     // Статистика
    SCREEN_CONFIRM,                    // Подтверждение действий
    SCREEN_HUMIDIFIER_SETTINGS,        // выбор режима увлажнителя
    SCREEN_HUMIDIFIER_THRESHOLD,       // настройка порога
    SCREEN_HUMIDIFIER_CYCLIC,          // настройка циклов
    SCREEN_LAMP_SETTINGS,               // настройки ламп (общее меню)
    SCREEN_LAMP1_SETTINGS,              // настройки лампы 1
    SCREEN_LAMP2_SETTINGS,              // настройки лампы 2
    SCREEN_LAMP3_SETTINGS                // настройки лампы 3
};

// Режим управления
enum ControlMode {
    MODE_AUTO,      // Автоматический (реле управляются датчиками)
    MODE_MANUAL     // Ручной (реле управляются с энкодера)
};

// Структура для порогов
struct Thresholds {
    int lightThreshold;
    int humidityThreshold;
    float tempThreshold;
};

// Структура для статистики
struct Statistics {
    float minTemp;
    float maxTemp;
    float minHum;
    float maxHum;
    float minLight;
    float maxLight;
    unsigned long uptime;
};

class Menu {
private:
    ScreenType _currentScreen;
    ScreenType _prevScreen;
    int _menuPosition;
    
    // ===== ЭНКОДЕР =====
    AiEsp32RotaryEncoder* _encoder;
    volatile long _encoderValue;
    bool _encoderPressed;
    unsigned long _pressStartTime;
    bool _longPressTriggered;
    
    // Режимы и настройки
    ControlMode _controlMode;
    Thresholds _thresholds;
    Thresholds _editThresholds;
    ScheduleManager* _scheduler;
    int _editingValue;
    bool _forceAutoUpdate;                // Флаг принудительного обновления AUTO
    
    // Флаги для переключения реле
    bool _manualToggleRequested;
    int _manualToggleIndex;

    // Для настройки увлажнителя
    int _editHumidifierMode;
    int _editHumidifierValue;
    
    // Внутренние методы
    void handleEncoderRotation(int delta);
    void handleShortPress();
    void handleLongPress();
    
    // Отрисовка экранов
    void drawMainScreen(float temp, float hum, float dsTemp, float light, RelayState relayState);
    void drawModeSelectScreen();
    void drawManualScreen(RelayState relayState);
    void drawHumidifierSettingsScreen();
    void drawHumidifierThresholdScreen();
    void drawHumidifierCyclicScreen();
    void drawLampSettingsScreen();
    void drawLampDetailScreen(int lampNumber);
    void drawSettingsScreen();
    void drawThresholdsScreen();
    void drawStatsScreen(Statistics stats);
    void drawConfirmScreen(const char* message);
    
public:
    Menu();
    void begin();
    void update(float temp, float hum, float dsTemp, float light, RelayState relayState, Statistics stats);
    
    // Для отладки
    void testEncoder();
    
    // IRAM_ATTR для обработки прерывания
    static void IRAM_ATTR readEncoderISR();
    
    // Геттеры
    ScreenType getCurrentScreen() { return _currentScreen; }
    ControlMode getControlMode() { return _controlMode; }
    Thresholds getThresholds() { return _thresholds; }
    
    bool isToggleRequested() { return _manualToggleRequested; }
    int getToggleIndex() { return _manualToggleIndex; }
    void clearToggleRequest() { _manualToggleRequested = false; }
    
    bool forceAutoUpdate() { return _forceAutoUpdate; }
    void clearForceAutoUpdate() { _forceAutoUpdate = false; }
    
    // Сеттеры
    void setScheduler(ScheduleManager* sched) { _scheduler = sched; }
    void setControlMode(ControlMode mode) { _controlMode = mode; }
    void setThresholds(Thresholds th) { _thresholds = th; }
};

#endif