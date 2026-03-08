#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Дни недели
enum WeekDay {
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 7
};

class TimeManager {
private:
    bool _timeSynced;
    unsigned long _lastSyncAttempt;
    struct tm _timeinfo;
    
public:
    TimeManager();
    bool begin();  // запускает NTP
    void update(); // вызывать в loop
    bool isTimeSynced() { return _timeSynced; }
    
    // Получение времени
    int getHour();
    int getMinute();
    int getSecond();
    int getDay();        // число месяца (1-31)
    int getMonth();      // месяц (1-12)
    int getYear();       // год (4 цифры)
    int getDayOfWeek();  // день недели (1-7, где 1 = понедельник)
    String getDayOfWeekStr(); // "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"
    String getFormattedTime(); // "14:25:36"
    String getFormattedDate(); // "23.03.2026"
    
    // Проверки для автоматизации
    bool isTimeBetween(int startHour, int startMin, int endHour, int endMin);
    bool isDayInList(int* days, int count);  // проверка, что сегодня один из указанных дней
};

#endif