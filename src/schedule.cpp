#include "schedule.h"
#include "config.h"
#include "logger.h"

ScheduleManager::ScheduleManager() {
    _humidifier.mode = HUMIDIFIER_CYCLIC;
    _humidifier.threshold = HUMIDITY_THRESHOLD;
    _humidifier.workTime = HUMIDIFIER_WORK_TIME;
    _humidifier.idleTime = HUMIDIFIER_IDLE_TIME;
    _humidifier.lastState = false;
    _humidifier.lastSwitch = 0;
}

void ScheduleManager::begin() {
    logger.log("РАСПИСАНИЕ", "✅ Инициализировано");
}

void ScheduleManager::update() {}

void ScheduleManager::setHumidifierThreshold(int threshold) {
    _humidifier.mode = HUMIDIFIER_THRESHOLD;
    _humidifier.threshold = threshold;
    logger.logf("УВЛАЖНИТЕЛЬ", "Режим: по порогу (%d%%)", threshold);
}

void ScheduleManager::setHumidifierCyclic(int workSec, int idleSec) {
    _humidifier.mode = HUMIDIFIER_CYCLIC;
    _humidifier.workTime = workSec;
    _humidifier.idleTime = idleSec;
    _humidifier.lastState = false;
    _humidifier.lastSwitch = millis();
    logger.logf("УВЛАЖНИТЕЛЬ", "Режим: циклический (%dс work / %dс idle)", workSec, idleSec);
}

bool ScheduleManager::shouldHumidifierRun(float humidity) {
    unsigned long now = millis();
    switch (_humidifier.mode) {
        case HUMIDIFIER_THRESHOLD:
            return (humidity < _humidifier.threshold);
        case HUMIDIFIER_CYCLIC: {
            unsigned long elapsed = now - _humidifier.lastSwitch;
            if (_humidifier.lastState) {
                if (elapsed >= _humidifier.workTime * 1000) {
                    _humidifier.lastState = false;
                    _humidifier.lastSwitch = now;
                }
            } else {
                if (elapsed >= _humidifier.idleTime * 1000) {
                    _humidifier.lastState = true;
                    _humidifier.lastSwitch = now;
                }
            }
            return _humidifier.lastState;
        }
        default:
            return false;
    }
}