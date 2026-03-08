#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"

struct RelayState {
    bool lamp1;
    bool lamp2;
    bool lamp3;
    bool humidifier;
};

class RelayController {
private:
    bool _lamp1State, _lamp2State, _lamp3State, _humidifierState;
    ChannelSettings _lamp1Settings, _lamp2Settings, _lamp3Settings, _humidifierSettings;
    GlobalSettings _globalSettings;

    bool shouldChannelBeOn(ChannelSettings& settings, float sensorValue, int hour, int min, int day, bool currentState);
    bool checkSensor(ChannelSettings& settings, float sensorValue, bool currentState);
    bool checkSchedule(ChannelSettings& settings, int hour, int min, int day);
    bool checkCycle(ChannelSettings& settings);

public:
    RelayController();
    void begin();
    void loadDefaults();
    void update(float light, float humidity, int hour, int min, int day);

    void setLamp1(bool state);
    void setLamp2(bool state);
    void setLamp3(bool state);
    void setAllLamps(bool state);
    void setHumidifier(bool state);
    void manualToggle(int index);
    void handleSerialCommand(char cmd);

    bool getLamp1() { return _lamp1State; }
    bool getLamp2() { return _lamp2State; }
    bool getLamp3() { return _lamp3State; }
    bool getHumidifier() { return _humidifierState; }
    RelayState getAllStates();

    ChannelSettings* getLamp1Settings() { return &_lamp1Settings; }
    ChannelSettings* getLamp2Settings() { return &_lamp2Settings; }
    ChannelSettings* getLamp3Settings() { return &_lamp3Settings; }
    ChannelSettings* getHumidifierSettings() { return &_humidifierSettings; }
    GlobalSettings* getGlobalSettings() { return &_globalSettings; }
};

#endif