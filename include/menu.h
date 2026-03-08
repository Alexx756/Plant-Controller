#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "config.h"
#include "relay.h"
#include "schedule.h"

enum ScreenType {
    SCREEN_MAIN,
    SCREEN_MODE_SELECT,
    SCREEN_MANUAL,
    SCREEN_SETTINGS,
    SCREEN_THRESHOLDS,
    SCREEN_STATS,
    SCREEN_CONFIRM,
    SCREEN_HUMIDIFIER_SETTINGS,
    SCREEN_HUMIDIFIER_THRESHOLD,
    SCREEN_HUMIDIFIER_CYCLIC,
    SCREEN_LAMP_SETTINGS,
    SCREEN_LAMP1_SETTINGS,
    SCREEN_LAMP2_SETTINGS,
    SCREEN_LAMP3_SETTINGS
};

enum ControlMode {
    MODE_AUTO,
    MODE_MANUAL
};

struct Thresholds {
    int lightThreshold;
    int humidityThreshold;
    float tempThreshold;
};

struct Statistics {
    float minTemp, maxTemp, minHum, maxHum, minLight, maxLight;
    unsigned long uptime;
};

class Menu {
private:
    ScreenType _currentScreen, _prevScreen;
    int _menuPosition;
    AiEsp32RotaryEncoder* _encoder;
    bool _encoderPressed;
    unsigned long _pressStartTime;
    bool _longPressTriggered;

    ControlMode _controlMode;
    Thresholds _thresholds;
    ScheduleManager* _scheduler;
    bool _forceAutoUpdate;
    bool _manualToggleRequested;
    int _manualToggleIndex;
    int _editHumidifierMode;
    int _editHumidifierValue;

    // для ламп
    RelayController* _relays;
    ChannelSettings _editLamp;
    int _editLampIndex;
    bool _editing;
    int _editingItem;

    void handleEncoderRotation(int delta);
    void handleShortPress();
    void handleLongPress();
    void handleLampPress();
    void handleLampEncoder(int delta);
    void saveCurrentLampSettings();

    void drawMainScreen(float temp, float hum, float dsTemp, float light, RelayState relayState);
    void drawModeSelectScreen();
    void drawManualScreen(RelayState relayState);
    void drawSettingsScreen();
    void drawThresholdsScreen();
    void drawStatsScreen(Statistics stats);
    void drawHumidifierSettingsScreen();
    void drawHumidifierThresholdScreen();
    void drawHumidifierCyclicScreen();
    void drawLampSettingsScreen();
    void drawLampDetailScreen(int lampNumber);
    void drawConfirmScreen(const char* message);

public:
    Menu();
    void begin();
    void update(float temp, float hum, float dsTemp, float light, RelayState relayState, Statistics stats);
    void testEncoder();
    static void IRAM_ATTR readEncoderISR();

    // Геттеры/сеттеры
    ScreenType getCurrentScreen() { return _currentScreen; }
    ControlMode getControlMode() { return _controlMode; }
    bool isToggleRequested() { return _manualToggleRequested; }
    int getToggleIndex() { return _manualToggleIndex; }
    void clearToggleRequest() { _manualToggleRequested = false; }
    bool forceAutoUpdate() { return _forceAutoUpdate; }
    void clearForceAutoUpdate() { _forceAutoUpdate = false; }
    void setScheduler(ScheduleManager* sched) { _scheduler = sched; }
    void setRelays(RelayController* r) { _relays = r; }
    void setControlMode(ControlMode mode) { _controlMode = mode; }
};

#endif