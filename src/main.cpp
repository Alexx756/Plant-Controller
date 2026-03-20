#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "display.h"
#include "sensors.h"
#include "relay.h"
#include "menu.h"
#include "time_manager.h"
#include "logger.h"  
#include "schedule.h"

// ============ ГЛОБАЛЬНЫЕ ОБЪЕКТЫ ============
Display display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
Sensors sensors(ONE_WIRE_BUS);
RelayController relays;
Menu menu;
TimeManager timeManager;
ScheduleManager scheduler;
LampScheduleEditor lampEditor;  // <-- ДОБАВЛЕНО

// ============ ПЕРЕМЕННЫЕ ============
SensorData sensorData;
Statistics stats;
unsigned long lastReadTime = 0;
unsigned long lastStatsUpdate = 0;

// ============ ИНИЦИАЛИЗАЦИЯ СТАТИСТИКИ ============
void initStats() {
    stats.minTemp = 100.0;
    stats.maxTemp = -100.0;
    stats.minHum = 100.0;
    stats.maxHum = 0.0;
    stats.minLight = 100000.0;
    stats.maxLight = 0.0;
    stats.uptime = 0;
    stats.tempValid = false;
    stats.humValid = false;
    stats.lightValid = false;
}

void updateStats(float temp, float hum, float light) {
    // Проверка на валидность чисел (не NaN, не Inf, не отрицательные для реальных физических величин)
    auto isValidNumber = [](float val) -> bool {
        return !isnan(val) && !isinf(val) && val >= -50 && val <= 150; // широкий диапазон
    };
    
    bool tempValid = isValidNumber(temp);
    bool humValid = isValidNumber(hum);
    bool lightValid = isValidNumber(light) && light > 0 && light < 100000; // свет > 0
    
    if (tempValid && humValid && sensorData.ahtValid) {
        if (temp < stats.minTemp) stats.minTemp = temp;
        if (temp > stats.maxTemp) stats.maxTemp = temp;
        if (hum < stats.minHum) stats.minHum = hum;
        if (hum > stats.maxHum) stats.maxHum = hum;
        stats.tempValid = true;
        stats.humValid = true;
    }
    if (lightValid && sensorData.lightValid) {
        if (light < stats.minLight) stats.minLight = light;
        if (light > stats.maxLight) stats.maxLight = light;
        stats.lightValid = true;
    }
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Инициализация логгера
    logger.begin();
    
    logger.log("СИСТЕМА", "Запуск Умного контроллера растений");
    
    // I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    logger.logf("I2C", "SDA=%d, SCL=%d", I2C_SDA, I2C_SCL);
    
    // Сканер I2C
    logger.log("I2C", "Сканирование шины...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            if (addr == 0x38) logger.log("I2C", "✅ AHT20 найден");
            else if (addr == 0x3C) logger.log("I2C", "✅ OLED найден");
            else if (addr == 0x23) logger.log("I2C", "✅ BH1750 найден");
            else logger.logf("I2C", "✅ Устройство 0x%02X", addr);
        }
    }
    
    // Дисплей
    if (display.begin()) {
        logger.log("ДИСПЛЕЙ", "✅ Инициализирован");
    } else {
        logger.log("ДИСПЛЕЙ", "❌ Ошибка");
    }
    
    // Датчики
    if (sensors.begin()) {
        logger.log("ДАТЧИКИ", "✅ Инициализированы");
    } else {
        logger.log("ДАТЧИКИ", "⚠️ Частичная инициализация");
    }
    
    // Реле
    relays.begin();
    logger.log("РЕЛЕ", "✅ Инициализированы");
    
    // Энкодер и меню
    menu.begin();
    menu.setScheduler(&scheduler);
    menu.setRelayController(&relays);
    menu.setLampEditor(&lampEditor);  // <-- ДОБАВЛЕНО
    logger.log("МЕНЮ", "✅ Инициализировано");
    
    // Подключаемся к Wi-Fi
    logger.log("Wi-Fi", "Подключение...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        logger.logf("Wi-Fi", "✅ Подключен, IP: %s", WiFi.localIP().toString().c_str());
        timeManager.begin();
    } else {
        logger.log("Wi-Fi", "❌ Не удалось подключиться");
    }
    
    // Прерывания для энкодера
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), Menu::readEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), Menu::readEncoderISR, CHANGE);
    logger.log("ЭНКОДЕР", "✅ Прерывания настроены");
    
    // Инициализация расписания
    scheduler.begin();

    relays.setScheduleManager(&scheduler);
    
    // Инициализация редактора ламп
    lampEditor.begin(&scheduler);  // <-- ДОБАВЛЕНО
    
    // Статистика
    initStats();
    logger.log("СИСТЕМА", "✅ Запуск завершен");
    
    delay(2000);
}

// ============ LOOP ============
void loop() {
    unsigned long now = millis();
    
    // Обновляем время
    timeManager.update();
    
    // Чтение датчиков
    if (now - lastReadTime > READ_INTERVAL) {
        sensorData = sensors.read();
        
        // Логирование датчиков (по таймеру из логгера)
        if (logger.shouldLog()) {
            // Время
            if (timeManager.isTimeSynced()) {
                logger.logTime(timeManager.getHour(), timeManager.getMinute(), timeManager.getSecond());
            }
            
            // Датчики
            if (sensorData.ahtValid) {
                logger.logSensors(
                    sensorData.ahtTemp,
                    sensorData.ahtHum,
                    sensorData.dsTemp,
                    sensorData.lightLevel
                );
            }
            
            // Реле
            RelayState rState = relays.getAllStates();
            logger.logRelay(
                rState.lamp1, rState.lamp2, rState.lamp3, rState.humidifier,
                menu.getControlMode()
            );
        }
        
        // ===== АВТОМАТИЧЕСКОЕ УПРАВЛЕНИЕ (AUTO режим) =====
        if (menu.getControlMode() == MODE_AUTO) {
            // getDayOfWeek() возвращает 0-6 (пн=0, вс=6) - соответствует индексам массивов
            relays.update(
                sensorData.lightLevel,
                sensorData.ahtHum,
                timeManager.getHour(),
                timeManager.getMinute(),
                timeManager.getDayOfWeek()
            );
        }
        
        // Принудительное обновление при входе в AUTO режим (если нужно)
        if (menu.forceAutoUpdate()) {
            relays.update(
                sensorData.lightLevel,
                sensorData.ahtHum,
                timeManager.getHour(),
                timeManager.getMinute(),
                timeManager.getDayOfWeek()
            );
            menu.clearForceAutoUpdate();
        }
        
        lastReadTime = now;
    }
    
    // Обработка запросов на переключение реле из меню
    if (menu.isToggleRequested()) {
        int idx = menu.getToggleIndex();
        relays.manualToggle(idx);
        menu.clearToggleRequest();
    }
    
    // Обработка команд из Serial
    if (Serial.available()) {
        char cmd = Serial.read();
        relays.handleSerialCommand(cmd);
    }
    
    // Обновление статистики (раз в минуту)
    if (now - lastStatsUpdate > 60000) {
        stats.uptime = millis() / 1000;
        if (sensorData.ahtValid) {
            updateStats(sensorData.ahtTemp, sensorData.ahtHum, sensorData.lightLevel);
        }
        lastStatsUpdate = now;
    }
    
    // Обновление меню и дисплея
    RelayState rState = relays.getAllStates();
    menu.update(
        sensorData.ahtTemp,
        sensorData.ahtHum,
        sensorData.dsTemp,
        sensorData.lightLevel,
        rState,
        stats
    );
    
    delay(10);
}