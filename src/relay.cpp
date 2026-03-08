#include "relay.h"
#include "logger.h"
#include <Arduino.h>

RelayController::RelayController() {
    // Инициализация по умолчанию
    _lamp1State = false;
    _lamp2State = false;
    _lamp3State = false;
    _humidifierState = false;
    
    loadDefaults();
}

void RelayController::loadDefaults() {
    // Лампы по умолчанию
    _lamp1Settings.mode = CH_MODE_SENSOR;
    _lamp1Settings.type = CH_TYPE_LAMP;
    _lamp1Settings.thresholdLow = LIGHT_THRESHOLD;
    _lamp1Settings.thresholdHigh = LIGHT_THRESHOLD + 50; // гистерезис 50 люкс
    _lamp1Settings.sensorHysteresis = 50;
    
    _lamp2Settings = _lamp1Settings;
    _lamp3Settings = _lamp1Settings;
    
    // Увлажнитель по умолчанию
    _humidifierSettings.mode = CH_MODE_SENSOR;
    _humidifierSettings.type = CH_TYPE_HUMIDIFIER;
    _humidifierSettings.thresholdLow = HUMIDITY_THRESHOLD;
    _humidifierSettings.thresholdHigh = HUMIDITY_THRESHOLD + 10; // +10% гистерезис
    _humidifierSettings.sensorHysteresis = 10;
    _humidifierSettings.cycleWorkTime = HUMIDIFIER_WORK_TIME;
    _humidifierSettings.cycleIdleTime = HUMIDIFIER_IDLE_TIME;
    _humidifierSettings.cycleEnabled = false;
    
    // Глобальные настройки
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
        // Для ламп: включаем ниже порога, выключаем выше порога+гистерезис
        if (currentState) {
            // Если лампа включена - выключаем только если свет выше порога+гистерезис
            return (sensorValue < settings.thresholdLow + settings.sensorHysteresis);
        } else {
            // Если лампа выключена - включаем если свет ниже порога
            return (sensorValue < settings.thresholdLow);
        }
    } else {
        // Для увлажнителя: включаем ниже порога, выключаем выше порога+гистерезис
        if (currentState) {
            return (sensorValue < settings.thresholdLow + settings.sensorHysteresis);
        } else {
            return (sensorValue < settings.thresholdLow);
        }
    }
}

bool RelayController::checkSchedule(ChannelSettings& settings, int hour, int min, int day) {
    // Проверка дня недели (индекс 0 = понедельник)
    if (day >= 0 && day < 7 && settings.scheduleDays[day] == false) {
        return false; // сегодня не разрешен
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
        // Сейчас работает - проверяем, не пора ли выключить
        if (now - settings.cycleLastSwitch > settings.cycleWorkTime * 1000) {
            settings.cycleLastState = false;
            settings.cycleLastSwitch = now;
        }
    } else {
        // Сейчас отдыхает - проверяем, не пора ли включить
        if (now - settings.cycleLastSwitch > settings.cycleIdleTime * 1000) {
            settings.cycleLastState = true;
            settings.cycleLastSwitch = now;
        }
    }
    return settings.cycleLastState;
}

bool RelayController::shouldChannelBeOn(ChannelSettings& settings, float sensorValue, int hour, int min, int day, bool currentState) {
    bool sensorResult = false;
    bool scheduleResult = false;
    bool cycleResult = false;
    
    // Проверка датчика
    if (settings.mode == CH_MODE_SENSOR || settings.mode == CH_MODE_SENSOR_SCHEDULE) {
        sensorResult = checkSensor(settings, sensorValue, currentState); // ← передаем состояние!
    }
    
    // Проверка расписания
    if (settings.mode == CH_MODE_SCHEDULE || settings.mode == CH_MODE_SENSOR_SCHEDULE) {
        scheduleResult = checkSchedule(settings, hour, min, day);
    }
    
    // Циклический режим (только для увлажнителя)
    if (settings.cycleEnabled && settings.type == CH_TYPE_HUMIDIFIER) {
        cycleResult = checkCycle(settings);
    }
    
    switch(settings.mode) {
        case CH_MODE_OFF:
            return false;
        case CH_MODE_ON:
            return true;
        case CH_MODE_SENSOR:
            return sensorResult;
        case CH_MODE_SCHEDULE:
            return scheduleResult;
        case CH_MODE_SENSOR_SCHEDULE:
            return sensorResult && scheduleResult;
        case CH_MODE_GLOBAL:
            // Использовать глобальные настройки
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
    // Расчет новых состояний для каждого канала с учетом текущего состояния
    bool newLamp1 = shouldChannelBeOn(_lamp1Settings, light, hour, min, day, _lamp1State);
    bool newLamp2 = shouldChannelBeOn(_lamp2Settings, light, hour, min, day, _lamp2State);
    bool newLamp3 = shouldChannelBeOn(_lamp3Settings, light, hour, min, day, _lamp3State);
    bool newHumidifier = shouldChannelBeOn(_humidifierSettings, humidity, hour, min, day, _humidifierState);
    
    // Применяем изменения
    if (newLamp1 != _lamp1State) setLamp1(newLamp1);
    if (newLamp2 != _lamp2State) setLamp2(newLamp2);
    if (newLamp3 != _lamp3State) setLamp3(newLamp3);
    if (newHumidifier != _humidifierState) setHumidifier(newHumidifier);
}

// Остальные методы без изменений (setLamp1, setLamp2, setAllLamps, manualToggle, handleSerialCommand)
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