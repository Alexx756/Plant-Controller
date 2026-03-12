#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"
#include "schedule.h"

struct RelayState {
    bool lamp1;
    bool lamp2;
    bool lamp3;
    bool humidifier;
};

class RelayController {
private:
    bool _lamp1State;
    bool _lamp2State;
    bool _lamp3State;
    bool _humidifierState;
    
    // Настройки каналов
    ChannelSettings _lamp1Settings;
    ChannelSettings _lamp2Settings;
    ChannelSettings _lamp3Settings;
    ChannelSettings _humidifierSettings;
    
    GlobalSettings _globalSettings;
    ScheduleManager* _schedManager;  // указатель на планировщик
    
    // Внутренние методы для расчета состояния
    bool shouldChannelBeOn(ChannelSettings& settings, float sensorValue, int hour, int min, int day, bool currentState);
    bool checkSensor(ChannelSettings& settings, float sensorValue, bool currentState);
    bool checkSchedule(ChannelSettings& settings, int hour, int min, int day);
    bool checkCycle(ChannelSettings& settings);
    
public:
    RelayController();
    void begin();
    void loadDefaults();  // загрузить настройки из config.h
    void update(float light, float humidity, int hour, int min, int day);
    
    // Ручное управление
    void setLamp1(bool state);
    void setLamp2(bool state);
    void setLamp3(bool state);
    void setAllLamps(bool state);
    void setHumidifier(bool state);
    void manualToggle(int index);
    void handleSerialCommand(char cmd);
    
    // Получение состояния
    bool getLamp1() { return _lamp1State; }
    bool getLamp2() { return _lamp2State; }
    bool getLamp3() { return _lamp3State; }
    bool getHumidifier() { return _humidifierState; }
    RelayState getAllStates();
    void setScheduleManager(ScheduleManager* sched) { _schedManager = sched; }
    
    //Настройки Увлажнителя

    void setHumidifierUseSensor(bool enable);
    void setHumidifierUseCyclic(bool enable);
    void setHumidifierThreshold(int low, int high, int hysteresis);
    void setHumidifierCycleTimes(int work, int idle);
    ChannelSettings* getHumidifierSettings() { return &_humidifierSettings; }

    // Доступ к настройкам для меню
    ChannelSettings* getLamp1Settings() { return &_lamp1Settings; }
    ChannelSettings* getLamp2Settings() { return &_lamp2Settings; }
    ChannelSettings* getLamp3Settings() { return &_lamp3Settings; }
    GlobalSettings* getGlobalSettings() { return &_globalSettings; }
    void setLampSchedule(int lamp, int startH, int startM, int endH, int endM, int* days, int daysCount);
};

#endif