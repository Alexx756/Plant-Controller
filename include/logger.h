#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "config.h"

class Logger {
private:
    unsigned long _lastLogTime;
    
public:
    Logger();
    void begin();
    bool shouldLog();  // проверяет интервал
    
    // Базовые методы логирования
    void log(const char* category, const char* message);
    void logf(const char* category, const char* format, ...);
    
    // Специализированные методы для разных компонентов
    void logSensors(float temp, float hum, float dsTemp, float light);
    void logRelay(bool lamp1, bool lamp2, bool lamp3, bool humidifier, int mode);
    void logAuto(const char* action, float value, int threshold);
    void logEncoder(int delta, int position);
    void logMenu(const char* action, int screen);
    void logTime(int hour, int minute, int second);
};

extern Logger logger;  // глобальный объект

#endif