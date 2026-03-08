#include "time_manager.h"
#include "config.h"
#include "logger.h"

TimeManager::TimeManager() { _timeSynced = false; }

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
    return _timeinfo.tm_mon + 1;
}

int TimeManager::getYear() {
    if (!_timeSynced) return 2024;
    return _timeinfo.tm_year + 1900;
}

int TimeManager::getDayOfWeek() {
    if (!_timeSynced) return 1;
    int wday = _timeinfo.tm_wday;
    return (wday == 0) ? 7 : wday;
}

String TimeManager::getFormattedTime() {
    if (!_timeSynced) return "--:--:--";
    char buf[9];
    strftime(buf, 9, "%H:%M:%S", &_timeinfo);
    return String(buf);
}

String TimeManager::getFormattedDate() {
    if (!_timeSynced) return "--.--.----";
    char buf[11];
    strftime(buf, 11, "%d.%m.%Y", &_timeinfo);
    return String(buf);
}

bool TimeManager::isTimeBetween(int startHour, int startMin, int endHour, int endMin) {
    if (!_timeSynced) return false;
    int now = getHour() * 60 + getMinute();
    int start = startHour * 60 + startMin;
    int end = endHour * 60 + endMin;
    if (start <= end) return (now >= start && now < end);
    else return (now >= start || now < end);
}