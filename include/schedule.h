#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <Arduino.h>

// Структура для хранения расписания одного устройства
struct ScheduleItem {
    bool enabled;
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
    int days[7];  // массив дней (1-7), 0 означает "не выбран"
    int daysCount;
};

// Режимы работы увлажнителя
enum HumidifierMode {
    HUMIDIFIER_THRESHOLD,  // по порогу влажности
    HUMIDIFIER_CYCLIC,     // циклический (работа/отдых)
    HUMIDIFIER_SCHEDULE     // по расписанию
};

// Структура для настроек увлажнителя
struct HumidifierSettings {
    HumidifierMode mode;
    int threshold;          // порог влажности для HUMIDIFIER_THRESHOLD
    int workTime;           // секунд работы (для циклического)
    int idleTime;           // секунд отдыха (для циклического)
    ScheduleItem schedule;  // расписание для HUMIDIFIER_SCHEDULE
    bool lastState;         // последнее состояние (для циклического)
    unsigned long lastSwitch; // время последнего переключения
};

class ScheduleManager {
private:
    ScheduleItem _lamp1Schedule;
    ScheduleItem _lamp2Schedule;
    ScheduleItem _lamp3Schedule;
    HumidifierSettings _humidifier;
    
    bool checkSchedule(ScheduleItem& sched, int currentHour, int currentMin, int currentDay);
    
public:
    ScheduleManager();
    void begin();
    void update();  // вызывать в loop
    
    // Настройка расписания для ламп
    void setLampSchedule(int lamp, int startH, int startM, int endH, int endM, int* days, int daysCount);
    bool isLampScheduled(int lamp, int hour, int min, int day);
    
    // Настройка увлажнителя
    void setHumidifierThreshold(int threshold);
    void setHumidifierCyclic(int workSec, int idleSec);
    void setHumidifierSchedule(int startH, int startM, int endH, int endM, int* days, int daysCount);
    bool shouldHumidifierRun(float currentHumidity);
    
    // Получение настроек
    HumidifierSettings getHumidifierSettings() { return _humidifier; }
};

#endif