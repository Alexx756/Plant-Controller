#include "time_manager.h"
#include "config.h"
#include "logger.h"

TimeManager::TimeManager() {
    _timeSynced = false;
    _lastSyncAttempt = 0;
}

bool TimeManager::begin() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    _timeSynced = getLocalTime(&_timeinfo);
    return _timeSynced;
}

void TimeManager::update() {
    _timeSynced = getLocalTime(&_timeinfo);
}

int TimeManager::getHour() {
    if (!_timeSynced) return 0;
    return _timeinfo.tm_hour;
}

int TimeManager::getMinute() {
    if (!_timeSynced) return 0;
    return _timeinfo.tm_min;
}

int TimeManager::getSecond() {
    if (!_timeSynced) return 0;
    return _timeinfo.tm_sec;
}

int TimeManager::getDay() {
    if (!_timeSynced) return 1;
    return _timeinfo.tm_mday;
}

int TimeManager::getMonth() {
    if (!_timeSynced) return 1;
    return _timeinfo.tm_mon + 1;  // tm_mon от 0 до 11
}

int TimeManager::getYear() {
    if (!_timeSynced) return 2024;
    return _timeinfo.tm_year + 1900;  // tm_year это годы с 1900
}

int TimeManager::getDayOfWeek() {
    if (!_timeSynced) return 1;
    // tm_wday: 0 = воскресенье, преобразуем в 1 = понедельник
    int wday = _timeinfo.tm_wday;
    if (wday == 0) return 7;  // воскресенье
    return wday;  // пн=1, вт=2, ср=3, чт=4, пт=5, сб=6
}

String TimeManager::getDayOfWeekStr() {
    int wday = getDayOfWeek();
    switch(wday) {
        case 1: return "ПН";
        case 2: return "ВТ";
        case 3: return "СР";
        case 4: return "ЧТ";
        case 5: return "ПТ";
        case 6: return "СБ";
        case 7: return "ВС";
        default: return "--";
    }
}

String TimeManager::getFormattedTime() {
    if (!_timeSynced) return "--:--:--";
    char buffer[9];
    strftime(buffer, 9, "%H:%M:%S", &_timeinfo);
    return String(buffer);
}

String TimeManager::getFormattedDate() {
    if (!_timeSynced) return "--.--.----";
    char buffer[11];
    strftime(buffer, 11, "%d.%m.%Y", &_timeinfo);
    return String(buffer);
}

bool TimeManager::isTimeBetween(int startHour, int startMin, int endHour, int endMin) {
    if (!_timeSynced) return false;
    
    int currentMinutes = getHour() * 60 + getMinute();
    int startMinutes = startHour * 60 + startMin;
    int endMinutes = endHour * 60 + endMin;
    
    if (startMinutes <= endMinutes) {
        return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
    } else {
        return (currentMinutes >= startMinutes || currentMinutes < endMinutes);
    }
}

bool TimeManager::isDayInList(int* days, int count) {
    if (!_timeSynced) return false;
    
    int today = getDayOfWeek();
    for (int i = 0; i < count; i++) {
        if (days[i] == today) return true;
    }
    return false;
}