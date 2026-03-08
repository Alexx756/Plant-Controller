#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class TimeManager {
private:
    bool _timeSynced;
    struct tm _timeinfo;
public:
    TimeManager();
    bool begin();
    void update();
    bool isTimeSynced() { return _timeSynced; }
    int getHour();
    int getMinute();
    int getSecond();
    int getDay();
    int getMonth();
    int getYear();
    int getDayOfWeek();
    String getFormattedTime();
    String getFormattedDate();
    bool isTimeBetween(int startHour, int startMin, int endHour, int endMin);
};

#endif