#include "logger.h"
#include <stdarg.h>

Logger::Logger() {
    _lastLogTime = 0;
}

void Logger::begin() {
    if (LOG_ENABLED) {
        Serial.println("📋 Логгер инициализирован");
        Serial.printf("📊 Интервал вывода: %d мс\n", LOG_INTERVAL);
    }
}

bool Logger::shouldLog() {
    if (!LOG_ENABLED) return false;
    
    unsigned long now = millis();
    if (now - _lastLogTime >= LOG_INTERVAL) {
        _lastLogTime = now;
        return true;
    }
    return false;
}

void Logger::log(const char* category, const char* message) {
    if (!LOG_ENABLED) return;
    Serial.printf("[%s] %s\n", category, message);
}

void Logger::logf(const char* category, const char* format, ...) {
    if (!LOG_ENABLED) return;
    
    Serial.print("[");
    Serial.print(category);
    Serial.print("] ");
    
    va_list args;
    va_start(args, format);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
    va_end(args);
    
    Serial.println();
}

void Logger::logSensors(float temp, float hum, float dsTemp, float light) {
    if (!LOG_ENABLED || !LOG_SENSORS) return;
    
    if (LOG_DETAILED) {
        logf("ДАТЧИКИ", "AHT20: %.1f°C %.1f%% | DS18B20: %.1f°C | BH1750: %.0f lx", 
             temp, hum, dsTemp, light);
    } else {
        logf("ДАТЧИКИ", "T%.1f H%.1f D%.1f L%.0f", temp, hum, dsTemp, light);
    }
}

void Logger::logRelay(bool lamp1, bool lamp2, bool lamp3, bool humidifier, int mode) {
    if (!LOG_ENABLED || !LOG_RELAY) return;
    
    const char* modeStr = (mode == 0) ? "AUTO" : "MANUAL";
    
    if (LOG_DETAILED) {
        logf("РЕЛЕ", "L1=%d L2=%d L3=%d H=%d | Режим: %s", 
             lamp1, lamp2, lamp3, humidifier, modeStr);
    } else {
        logf("РЕЛЕ", "%d%d%d%d %s", lamp1, lamp2, lamp3, humidifier, modeStr);
    }
}

void Logger::logAuto(const char* action, float value, int threshold) {
    if (!LOG_ENABLED || !LOG_AUTO) return;
    
    logf("AUTO", "%s: %.1f (порог %d)", action, value, threshold);
}

void Logger::logEncoder(int delta, int position) {
    if (!LOG_ENABLED || !LOG_ENCODER) return;
    
    logf("ЭНКОДЕР", "delta=%d pos=%d", delta, position);
}

void Logger::logMenu(const char* action, int screen) {
    if (!LOG_ENABLED || !LOG_MENU) return;
    
    logf("МЕНЮ", "%s на экране %d", action, screen);
}

void Logger::logTime(int hour, int minute, int second) {
    if (!LOG_ENABLED || !LOG_TIME) return;
    
    logf("ВРЕМЯ", "%02d:%02d:%02d", hour, minute, second);
}

void Logger::logDebug(const char* format, ...) {
    if (!LOG_ENABLED || !LOG_DEBUG) return;
    
    Serial.print("[DEBUG] ");
    
    va_list args;
    va_start(args, format);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
    va_end(args);
    
    Serial.println();
}

Logger logger;  // глобальный экземпляр
