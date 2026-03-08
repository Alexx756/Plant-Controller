#include "relay.h"
#include "logger.h"
#include <Arduino.h>

RelayController::RelayController() {
    _lamp1State = _lamp2State = _lamp3State = _humidifierState = false;
    loadDefaults();
}

void RelayController::loadDefaults() {
    // Лампы
    _lamp1Settings.mode = CH_MODE_SENSOR;
    _lamp1Settings.type = CH_TYPE_LAMP;
    _lamp1Settings.thresholdLow = LIGHT_THRESHOLD;
    _lamp1Settings.thresholdHigh = LIGHT_THRESHOLD + 50;
    _lamp1Settings.sensorHysteresis = 50;
    _lamp2Settings = _lamp3Settings = _lamp1Settings;

    // Увлажнитель
    _humidifierSettings.mode = CH_MODE_SENSOR;
    _humidifierSettings.type = CH_TYPE_HUMIDIFIER;
    _humidifierSettings.thresholdLow = HUMIDITY_THRESHOLD;
    _humidifierSettings.thresholdHigh = HUMIDITY_THRESHOLD + 10;
    _humidifierSettings.sensorHysteresis = 10;
    _humidifierSettings.cycleWorkTime = HUMIDIFIER_WORK_TIME;
    _humidifierSettings.cycleIdleTime = HUMIDIFIER_IDLE_TIME;
    _humidifierSettings.cycleEnabled = false;
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
    if (currentState)
        return (sensorValue < settings.thresholdLow + settings.sensorHysteresis);
    else
        return (sensorValue < settings.thresholdLow);
}

bool RelayController::checkSchedule(ChannelSettings& settings, int hour, int min, int day) {
    if (day >= 0 && day < 7 && !settings.scheduleDays[day]) return false;
    int now = hour * 60 + min;
    int start = settings.scheduleStartHour * 60 + settings.scheduleStartMin;
    int end = settings.scheduleEndHour * 60 + settings.scheduleEndMin;
    if (start <= end) return (now >= start && now < end);
    else return (now >= start || now < end);
}

bool RelayController::shouldChannelBeOn(ChannelSettings& settings, float sensorValue, int hour, int min, int day, bool currentState) {
    bool sensorRes = false, scheduleRes = false;
    if (settings.mode == CH_MODE_SENSOR || settings.mode == CH_MODE_SENSOR_SCHEDULE)
        sensorRes = checkSensor(settings, sensorValue, currentState);
    if (settings.mode == CH_MODE_SCHEDULE || settings.mode == CH_MODE_SENSOR_SCHEDULE)
        scheduleRes = checkSchedule(settings, hour, min, day);

    switch (settings.mode) {
        case CH_MODE_OFF: return false;
        case CH_MODE_ON: return true;
        case CH_MODE_SENSOR: return sensorRes;
        case CH_MODE_SCHEDULE: return scheduleRes;
        case CH_MODE_SENSOR_SCHEDULE: return sensorRes && scheduleRes;
        default: return false;
    }
}

void RelayController::update(float light, float humidity, int hour, int min, int day) {
    bool newL1 = shouldChannelBeOn(_lamp1Settings, light, hour, min, day, _lamp1State);
    bool newL2 = shouldChannelBeOn(_lamp2Settings, light, hour, min, day, _lamp2State);
    bool newL3 = shouldChannelBeOn(_lamp3Settings, light, hour, min, day, _lamp3State);
    bool newHum = shouldChannelBeOn(_humidifierSettings, humidity, hour, min, day, _humidifierState);
    if (newL1 != _lamp1State) setLamp1(newL1);
    if (newL2 != _lamp2State) setLamp2(newL2);
    if (newL3 != _lamp3State) setLamp3(newL3);
    if (newHum != _humidifierState) setHumidifier(newHum);
}

void RelayController::setLamp1(bool state) { digitalWrite(RELAY_LAMP1, state); _lamp1State = state; logger.logf("РЕЛЕ", "Лампа1: %s", state ? "ON" : "OFF"); }
void RelayController::setLamp2(bool state) { digitalWrite(RELAY_LAMP2, state); _lamp2State = state; logger.logf("РЕЛЕ", "Лампа2: %s", state ? "ON" : "OFF"); }
void RelayController::setLamp3(bool state) { digitalWrite(RELAY_LAMP3, state); _lamp3State = state; logger.logf("РЕЛЕ", "Лампа3: %s", state ? "ON" : "OFF"); }
void RelayController::setAllLamps(bool state) { setLamp1(state); setLamp2(state); setLamp3(state); }
void RelayController::setHumidifier(bool state) { digitalWrite(RELAY_HUMIDIFIER, state); _humidifierState = state; logger.logf("РЕЛЕ", "Увлажнитель: %s", state ? "ON" : "OFF"); }

RelayState RelayController::getAllStates() {
    RelayState s;
    s.lamp1 = _lamp1State; s.lamp2 = _lamp2State; s.lamp3 = _lamp3State; s.humidifier = _humidifierState;
    return s;
}

void RelayController::manualToggle(int index) {
    switch (index) {
        case 0: setLamp1(!_lamp1State); break;
        case 1: setLamp2(!_lamp2State); break;
        case 2: setLamp3(!_lamp3State); break;
        case 3: setHumidifier(!_humidifierState); break;
    }
}

void RelayController::handleSerialCommand(char cmd) {
    switch (cmd) {
        case '1': setLamp1(!_lamp1State); break;
        case '2': setLamp2(!_lamp2State); break;
        case '3': setLamp3(!_lamp3State); break;
        case 'a': setAllLamps(!_lamp1State); break;
        case 'h': setHumidifier(!_humidifierState); break;
        case 's': {
            RelayState s = getAllStates();
            logger.logf("СТАТУС", "L1=%d L2=%d L3=%d H=%d", s.lamp1, s.lamp2, s.lamp3, s.humidifier);
            break;
        }
    }
}