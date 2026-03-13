#include "schedule.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>

ScheduleManager::ScheduleManager() {
    // Инициализация расписаний по умолчанию
    _lamp1Schedule.enabled = false;
    _lamp2Schedule.enabled = false;
    _lamp3Schedule.enabled = false;
    
    // Настройки увлажнителя по умолчанию
    _humidifier.mode = HUMIDIFIER_CYCLIC;
    _humidifier.threshold = HUMIDITY_THRESHOLD;
    _humidifier.workTime = HUMIDIFIER_WORK_TIME;
    _humidifier.idleTime = HUMIDIFIER_IDLE_TIME;
    _humidifier.schedule.enabled = false;
    _humidifier.lastState = false;
    _humidifier.lastSwitch = 0;
}

void ScheduleManager::begin() {
    logger.log("РАСПИСАНИЕ", "✅ Инициализировано");
}

void ScheduleManager::update() {
    // Здесь можно добавить логику, если нужно
}

bool ScheduleManager::checkSchedule(ScheduleItem& sched, int currentHour, int currentMin, int currentDay) {
    if (!sched.enabled) return false;
    
    // Проверка дня недели
    bool dayOk = (sched.daysCount == 0);  // если дни не указаны - любой день
    for (int i = 0; i < sched.daysCount; i++) {
        if (sched.days[i] == currentDay) {
            dayOk = true;
            break;
        }
    }
    if (!dayOk) return false;
    
    // Проверка времени
    int currentMinutes = currentHour * 60 + currentMin;
    int startMinutes = sched.startHour * 60 + sched.startMinute;
    int endMinutes = sched.endHour * 60 + sched.endMinute;
    
    if (startMinutes <= endMinutes) {
        return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
    } else {
        return (currentMinutes >= startMinutes || currentMinutes < endMinutes);
    }
}

void ScheduleManager::setLampSchedule(int lamp, int startH, int startM, int endH, int endM, int* days, int daysCount) {
    ScheduleItem* sched = nullptr;
    switch(lamp) {
        case 1: sched = &_lamp1Schedule; break;
        case 2: sched = &_lamp2Schedule; break;
        case 3: sched = &_lamp3Schedule; break;
        default: return;
    }
    
    sched->enabled = true;
    sched->startHour = startH;
    sched->startMinute = startM;
    sched->endHour = endH;
    sched->endMinute = endM;
    sched->daysCount = daysCount;
    for (int i = 0; i < daysCount; i++) {
        sched->days[i] = days[i];
    }
    
    logger.logf("РАСПИСАНИЕ", "Лампа %d: %02d:%02d - %02d:%02d", 
                lamp, startH, startM, endH, endM);
}

bool ScheduleManager::isLampScheduled(int lamp, int hour, int min, int day) {
    switch(lamp) {
        case 1: return checkSchedule(_lamp1Schedule, hour, min, day);
        case 2: return checkSchedule(_lamp2Schedule, hour, min, day);
        case 3: return checkSchedule(_lamp3Schedule, hour, min, day);
        default: return false;
    }
}

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

void ScheduleManager::setHumidifierSchedule(int startH, int startM, int endH, int endM, int* days, int daysCount) {
    _humidifier.mode = HUMIDIFIER_SCHEDULE;
    _humidifier.schedule.enabled = true;
    _humidifier.schedule.startHour = startH;
    _humidifier.schedule.startMinute = startM;
    _humidifier.schedule.endHour = endH;
    _humidifier.schedule.endMinute = endM;
    _humidifier.schedule.daysCount = daysCount;
    for (int i = 0; i < daysCount; i++) {
        _humidifier.schedule.days[i] = days[i];
    }
    logger.logf("УВЛАЖНИТЕЛЬ", "Режим: по расписанию %02d:%02d - %02d:%02d", 
                startH, startM, endH, endM);
}

bool ScheduleManager::shouldHumidifierRun(float currentHumidity) {
    unsigned long now = millis();
    
    switch(_humidifier.mode) {
        case HUMIDIFIER_THRESHOLD:
            return (currentHumidity < _humidifier.threshold);
            
        case HUMIDIFIER_CYCLIC: {
            unsigned long elapsed = now - _humidifier.lastSwitch;
            if (_humidifier.lastState) {
                // Сейчас работает - проверяем, не пора ли выключить
                if (elapsed >= _humidifier.workTime * 1000) {
                    _humidifier.lastState = false;
                    _humidifier.lastSwitch = now;
                }
            } else {
                // Сейчас отдыхает - проверяем, не пора ли включить
                if (elapsed >= _humidifier.idleTime * 1000) {
                    _humidifier.lastState = true;
                    _humidifier.lastSwitch = now;
                }
            }
            return _humidifier.lastState;
        }
            
        case HUMIDIFIER_SCHEDULE:
            // Здесь будет проверка по расписанию (нужны время и день)
            return false;  // временно
            
        default:
            return false;
    }
}

bool ScheduleManager::isHumidifierScheduled(int hour, int min, int day) {
    return checkSchedule(_humidifier.schedule, hour, min, day);
}

// ============ РЕАЛИЗАЦИЯ LampScheduleEditor ============

LampScheduleEditor::LampScheduleEditor() {
    _schedManager = nullptr;
    _editMode = 0;
    _editTimeMode = false;
    _editHour = 0;
    _editMinute = 0;
    _editing = false;
}

void LampScheduleEditor::begin(ScheduleManager* sched) {
    _schedManager = sched;
}

void LampScheduleEditor::startEditing(int lampNumber, bool useSensor, bool useSchedule, 
                                      int threshold, int hysteresis,
                                      const bool days[7], int startHour, int startMin, 
                                      int endHour, int endMin) {
    _lampNumber = lampNumber;
    _useSensor = useSensor;
    _useSchedule = useSchedule;
    _threshold = threshold;
    _hysteresis = hysteresis;
    _scheduleStartHour = startHour;
    _scheduleStartMin = startMin;
    _scheduleEndHour = endHour;
    _scheduleEndMin = endMin;
    
    for (int i = 0; i < 7; i++) {
        _scheduleDays[i] = days[i];
    }
    
    _editMode = 0;  // начинаем с дней
    _editTimeMode = false;
    _editHour = _scheduleStartHour;
    _editMinute = _scheduleStartMin;
    _editing = false;
}

void LampScheduleEditor::navigate(int delta) {
    if (_editMode == 0) {
        // Навигация по дням недели (управляется из menu.cpp через toggleDay)
        // Здесь ничего не делаем
    } else if (_editMode == 1) {
        // Редактирование времени начала
        if (_editing) {
            if (!_editTimeMode) {
                // Редактируем часы
                _editHour += delta;
                if (_editHour < 0) _editHour = 23;
                if (_editHour > 23) _editHour = 0;
            } else {
                // Редактируем минуты
                _editMinute += delta;
                if (_editMinute < 0) _editMinute = 59;
                if (_editMinute > 59) _editMinute = 0;
            }
        }
    } else if (_editMode == 2) {
        // Редактирование времени конца
        if (_editing) {
            if (!_editTimeMode) {
                _editHour += delta;
                if (_editHour < 0) _editHour = 23;
                if (_editHour > 23) _editHour = 0;
            } else {
                _editMinute += delta;
                if (_editMinute < 0) _editMinute = 59;
                if (_editMinute > 59) _editMinute = 0;
            }
        }
    }
}

void LampScheduleEditor::setEditMode(int mode) {
    if (mode >= 0 && mode <= 2) {
        _editMode = mode;
        _editing = false;
        _editTimeMode = false;
        // Инициализируем editHour/editMinute текущими значениями
        if (mode == 1) {
            _editHour = _scheduleStartHour;
            _editMinute = _scheduleStartMin;
        } else if (mode == 2) {
            _editHour = _scheduleEndHour;
            _editMinute = _scheduleEndMin;
        }
    }
}

void LampScheduleEditor::switchEditMode() {
    if (_editMode < 2) {
        _editMode++;
    } else {
        _editMode = 0;
    }
    _editing = false;
    _editTimeMode = false;
    
    // При переключении на редактирование времени, инициализируем editHour/editMinute
    if (_editMode == 1) {
        _editHour = _scheduleStartHour;
        _editMinute = _scheduleStartMin;
    } else if (_editMode == 2) {
        _editHour = _scheduleEndHour;
        _editMinute = _scheduleEndMin;
    }
}

void LampScheduleEditor::toggleDay(int dayIndex) {
    if (dayIndex >= 0 && dayIndex < 7) {
        _scheduleDays[dayIndex] = !_scheduleDays[dayIndex];
    }
}

void LampScheduleEditor::startEditingTime() {
    _editing = true;
    _editTimeMode = false;  // начинаем с часов
    // Устанавливаем текущие значения
    if (_editMode == 1) {
        _editHour = _scheduleStartHour;
        _editMinute = _scheduleStartMin;
    } else if (_editMode == 2) {
        _editHour = _scheduleEndHour;
        _editMinute = _scheduleEndMin;
    }
}

void LampScheduleEditor::toggleTimeUnit() {
    _editTimeMode = !_editTimeMode;
}

void LampScheduleEditor::saveAndExit() {
    // Сохраняем отредактированные времена
    if (_editMode == 1) {
        _scheduleStartHour = _editHour;
        _scheduleStartMin = _editMinute;
    } else if (_editMode == 2) {
        _scheduleEndHour = _editHour;
        _scheduleEndMin = _editMinute;
    }
    _editing = false;
}

void LampScheduleEditor::editTime(int delta) {
    navigate(delta);
}

void LampScheduleEditor::toggleTimeMode() {
    _editTimeMode = !_editTimeMode;
}

void LampScheduleEditor::toggleUseSensor() {
    _useSensor = !_useSensor;
}

void LampScheduleEditor::toggleUseSchedule() {
    _useSchedule = !_useSchedule;
}

void LampScheduleEditor::changeThreshold(int delta) {
    _threshold += delta;
    if (_threshold < 0) _threshold = 0;
    if (_threshold > 1000) _threshold = 1000;
}

void LampScheduleEditor::changeHysteresis(int delta) {
    _hysteresis += delta;
    if (_hysteresis < 0) _hysteresis = 0;
    if (_hysteresis > 100) _hysteresis = 100;
}

void LampScheduleEditor::saveToSchedule() {
    if (_schedManager == nullptr) return;
    
    // Сохраняем отредактированные времена (на случай, если редактирование не было завершено)
    if (_editing) {
        if (_editMode == 1) {
            _scheduleStartHour = _editHour;
            _scheduleStartMin = _editMinute;
        } else if (_editMode == 2) {
            _scheduleEndHour = _editHour;
            _scheduleEndMin = _editMinute;
        }
    }
    
    // Преобразуем bool массив в int массив выбранных дней
    int selectedDays[7];
    int daysCount = 0;
    for (int i = 0; i < 7; i++) {
        if (_scheduleDays[i]) {
            selectedDays[daysCount++] = i;
        }
    }
    
    // Сохраняем в планировщик
    _schedManager->setLampSchedule(
        _lampNumber,
        _scheduleStartHour, _scheduleStartMin,
        _scheduleEndHour, _scheduleEndMin,
        selectedDays, daysCount
    );
    
    logger.logf("МЕНЮ", "Расписание лампы %d сохранено", _lampNumber);
}