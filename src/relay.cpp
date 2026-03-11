#include "relay.h"
#include "logger.h"
#include <Arduino.h>

RelayController::RelayController() {
    // Инициализация по умолчанию
    _lamp1State = false;
    _lamp2State = false;
    _lamp3State = false;
    _humidifierState = false;
    _schedManager = nullptr;
    
    loadDefaults();
}

void RelayController::loadDefaults() {
    // ===== Лампа 1 =====
    _lamp1Settings.index = 1;
    _lamp1Settings.mode = CH_MODE_SENSOR;
    _lamp1Settings.type = CH_TYPE_LAMP; 
    _lamp1Settings.useSensor = true;
    _lamp1Settings.useCyclic = false;
    _lamp1Settings.useSchedule = false;
    _lamp1Settings.thresholdLow = LIGHT_THRESHOLD;
    _lamp1Settings.thresholdHigh = LIGHT_THRESHOLD + 50;
    _lamp1Settings.sensorHysteresis = 50;
    _lamp1Settings.cycleEnabled = false;
    
    // ===== Лампа 2 =====
    _lamp2Settings.index = 2;
    _lamp2Settings.mode = CH_MODE_SENSOR;
    _lamp2Settings.type = CH_TYPE_LAMP; 
    _lamp2Settings.useSensor = true;
    _lamp2Settings.useCyclic = false;
    _lamp2Settings.useSchedule = false;
    _lamp2Settings.thresholdLow = LIGHT_THRESHOLD;
    _lamp2Settings.thresholdHigh = LIGHT_THRESHOLD + 50;
    _lamp2Settings.sensorHysteresis = 50;
    _lamp2Settings.cycleEnabled = false;
    
    // ===== Лампа 3 =====
    _lamp3Settings.index = 3;
    _lamp3Settings.mode = CH_MODE_SENSOR;
    _lamp3Settings.type = CH_TYPE_LAMP; 
    _lamp3Settings.useSensor = true;
    _lamp3Settings.useCyclic = false;
    _lamp3Settings.useSchedule = false;
    _lamp3Settings.thresholdLow = LIGHT_THRESHOLD;
    _lamp3Settings.thresholdHigh = LIGHT_THRESHOLD + 50;
    _lamp3Settings.sensorHysteresis = 50;
    _lamp3Settings.cycleEnabled = false;
    
    // ===== Увлажнитель =====
    _humidifierSettings.index = 4;
    _humidifierSettings.type = CH_TYPE_HUMIDIFIER; 
    _humidifierSettings.mode = CH_MODE_SENSOR;
    _humidifierSettings.useSensor = true;
    #ifdef HUMIDIFIER_MODE_CYCLIC
        _humidifierSettings.useCyclic = HUMIDIFIER_MODE_CYCLIC;
    #else
        _humidifierSettings.useCyclic = false;
    #endif
    _humidifierSettings.useSchedule = false;
    
    _humidifierSettings.thresholdLow = HUMIDITY_THRESHOLD;
    _humidifierSettings.thresholdHigh = HUMIDITY_THRESHOLD + 10;
    _humidifierSettings.sensorHysteresis = 10;
    _humidifierSettings.cycleWorkTime = HUMIDIFIER_WORK_TIME;
    _humidifierSettings.cycleIdleTime = HUMIDIFIER_IDLE_TIME;
    _humidifierSettings.cycleEnabled = _humidifierSettings.useCyclic;
    
    // ===== Глобальные настройки =====
    _globalSettings.globalLightThreshold = LIGHT_THRESHOLD;
    _globalSettings.globalLightHysteresis = 50;
    _globalSettings.globalHumidityThreshold = HUMIDITY_THRESHOLD;
    _globalSettings.globalHumidityHysteresis = 10;
    _globalSettings.globalScheduleEnabled = false;
}

void RelayController::begin() {
    pinMode(RELAY_LAMP1, OUTPUT);
    pinMode(RELAY_LAMP2, OUTPUT);
    pinMode(RELAY_LAMP3, OUTPUT);
    pinMode(RELAY_HUMIDIFIER, OUTPUT);
    
    setAllLamps(false);
    setHumidifier(false);
    
    logger.log("РЕЛЕ", "✅ Инициализированы");
}

bool RelayController::checkSensor(ChannelSettings& settings, float sensorValue, bool currentState) {
    if (settings.type == CH_TYPE_LAMP) {
        if (currentState) {
            return (sensorValue < settings.thresholdLow + settings.sensorHysteresis);
        } else {
            return (sensorValue < settings.thresholdLow);
        }
    } else {
        if (currentState) {
            return (sensorValue < settings.thresholdLow + settings.sensorHysteresis);
        } else {
            return (sensorValue < settings.thresholdLow);
        }
    }
}

bool RelayController::checkSchedule(ChannelSettings& settings, int hour, int min, int day) {
    if (day >= 0 && day < 7 && settings.scheduleDays[day] == false) {
        return false;
    }
    
    int currentMinutes = hour * 60 + min;
    int startMinutes = settings.scheduleStartHour * 60 + settings.scheduleStartMin;
    int endMinutes = settings.scheduleEndHour * 60 + settings.scheduleEndMin;
    
    if (startMinutes <= endMinutes) {
        return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
    } else {
        return (currentMinutes >= startMinutes || currentMinutes < endMinutes);
    }
}

bool RelayController::checkCycle(ChannelSettings& settings) {
    unsigned long now = millis();
    
    if (settings.cycleLastState) {
        if (now - settings.cycleLastSwitch > settings.cycleWorkTime * 1000) {
            settings.cycleLastState = false;
            settings.cycleLastSwitch = now;
        }
    } else {
        if (now - settings.cycleLastSwitch > settings.cycleIdleTime * 1000) {
            settings.cycleLastState = true;
            settings.cycleLastSwitch = now;
        }
    }
    return settings.cycleLastState;
}

bool RelayController::shouldChannelBeOn(ChannelSettings& settings, float sensorValue, int hour, int min, int day, bool currentState) {
    if (settings.useSensor || settings.useCyclic || settings.useSchedule) {
        bool result = true;
        
        if (settings.useCyclic) {
            result = result && checkCycle(settings);
        }
        
        if (settings.useSensor) {
            result = result && checkSensor(settings, sensorValue, currentState);
        }
        
        if (settings.useSchedule && _schedManager != nullptr) {
            bool scheduleOk = false;
            if (settings.type == CH_TYPE_HUMIDIFIER) {
                scheduleOk = _schedManager->isHumidifierScheduled(hour, min, day);
            } else {
                scheduleOk = _schedManager->isLampScheduled(settings.index, hour, min, day);
            }
            result = result && scheduleOk;
        }
        
        return result;
    }
    
    switch(settings.mode) {
        case CH_MODE_OFF:
            return false;
        case CH_MODE_ON:
            return true;
        case CH_MODE_SENSOR:
            return checkSensor(settings, sensorValue, currentState);
        case CH_MODE_SCHEDULE:
            if (_schedManager == nullptr) return false;
            if (settings.type == CH_TYPE_HUMIDIFIER) {
                return _schedManager->isHumidifierScheduled(hour, min, day);
            } else {
                return _schedManager->isLampScheduled(settings.index, hour, min, day);
            }
        case CH_MODE_SENSOR_SCHEDULE:
            if (_schedManager == nullptr) return false;
            {
                bool sensorPart = checkSensor(settings, sensorValue, currentState);
                bool schedPart = (settings.type == CH_TYPE_HUMIDIFIER) ?
                    _schedManager->isHumidifierScheduled(hour, min, day) :
                    _schedManager->isLampScheduled(settings.index, hour, min, day);
                return sensorPart && schedPart;
            }
        case CH_MODE_GLOBAL:
            if (settings.type == CH_TYPE_LAMP) {
                return (sensorValue < _globalSettings.globalLightThreshold);
            } else {
                return (sensorValue < _globalSettings.globalHumidityThreshold);
            }
        default:
            return false;
    }
}

void RelayController::update(float light, float humidity, int hour, int min, int day) {
    logger.logDebug("update: useSensor=%d, useCyclic=%d, useSchedule=%d",
                  _humidifierSettings.useSensor,
                  _humidifierSettings.useCyclic,
                  _humidifierSettings.useSchedule);
    
    bool newLamp1 = shouldChannelBeOn(_lamp1Settings, light, hour, min, day, _lamp1State);
    bool newLamp2 = shouldChannelBeOn(_lamp2Settings, light, hour, min, day, _lamp2State);
    bool newLamp3 = shouldChannelBeOn(_lamp3Settings, light, hour, min, day, _lamp3State);
    bool newHumidifier = shouldChannelBeOn(_humidifierSettings, humidity, hour, min, day, _humidifierState);
    
    if (newLamp1 != _lamp1State) setLamp1(newLamp1);
    if (newLamp2 != _lamp2State) setLamp2(newLamp2);
    if (newLamp3 != _lamp3State) setLamp3(newLamp3);
    if (newHumidifier != _humidifierState) setHumidifier(newHumidifier);
}

void RelayController::setLamp1(bool state) {
    digitalWrite(RELAY_LAMP1, state ? HIGH : LOW);
    _lamp1State = state;
    logger.logf("РЕЛЕ", "Лампа1: %s", state ? "ON" : "OFF");
}

void RelayController::setLamp2(bool state) {
    digitalWrite(RELAY_LAMP2, state ? HIGH : LOW);
    _lamp2State = state;
    logger.logf("РЕЛЕ", "Лампа2: %s", state ? "ON" : "OFF");
}

void RelayController::setLamp3(bool state) {
    digitalWrite(RELAY_LAMP3, state ? HIGH : LOW);
    _lamp3State = state;
    logger.logf("РЕЛЕ", "Лампа3: %s", state ? "ON" : "OFF");
}

void RelayController::setAllLamps(bool state) {
    setLamp1(state);
    setLamp2(state);
    setLamp3(state);
    logger.logf("РЕЛЕ", "Все лампы: %s", state ? "ON" : "OFF");
}

void RelayController::setHumidifier(bool state) {
    digitalWrite(RELAY_HUMIDIFIER, state ? HIGH : LOW);
    _humidifierState = state;
    logger.logf("РЕЛЕ", "Увлажнитель: %s", state ? "ON" : "OFF");
}

void RelayController::setHumidifierUseSensor(bool enable) {
    _humidifierSettings.useSensor = enable;
    logger.logDebug("setHumidifierUseSensor(%d)", enable);
    logger.logf("РЕЛЕ", "useSensor = %d", enable);
}

void RelayController::setHumidifierUseCyclic(bool enable) {
    _humidifierSettings.useCyclic = enable;
    _humidifierSettings.cycleEnabled = enable;
    logger.logDebug("setHumidifierUseCyclic(%d)", enable);
    logger.logf("РЕЛЕ", "useCyclic = %d", enable);
}

void RelayController::setHumidifierThreshold(int low, int high, int hysteresis) {
    _humidifierSettings.thresholdLow = low;
    _humidifierSettings.thresholdHigh = high;
    _humidifierSettings.sensorHysteresis = hysteresis;
    logger.logDebug("setHumidifierThreshold low=%d high=%d hyst=%d", low, high, hysteresis);
}

void RelayController::setHumidifierCycleTimes(int work, int idle) {
    _humidifierSettings.cycleWorkTime = work;
    _humidifierSettings.cycleIdleTime = idle;
    logger.logDebug("setHumidifierCycleTimes work=%d idle=%d", work, idle);
}

RelayState RelayController::getAllStates() {
    RelayState state;
    state.lamp1 = _lamp1State;
    state.lamp2 = _lamp2State;
    state.lamp3 = _lamp3State;
    state.humidifier = _humidifierState;
    return state;
}

void RelayController::manualToggle(int index) {
    switch(index) {
        case 0: setLamp1(!_lamp1State); break;
        case 1: setLamp2(!_lamp2State); break;
        case 2: setLamp3(!_lamp3State); break;
        case 3: setHumidifier(!_humidifierState); break;
        default: break;
    }
}

void RelayController::handleSerialCommand(char cmd) {
    switch(cmd) {
        case '1': setLamp1(!_lamp1State); break;
        case '2': setLamp2(!_lamp2State); break;
        case '3': setLamp3(!_lamp3State); break;
        case 'a': setAllLamps(!_lamp1State); break;
        case 'h': setHumidifier(!_humidifierState); break;
        case 's': {
            RelayState s = getAllStates();
            logger.logf("СТАТУС", "L1=%d L2=%d L3=%d H=%d", 
                       s.lamp1, s.lamp2, s.lamp3, s.humidifier);
            break;
        }
    }
}