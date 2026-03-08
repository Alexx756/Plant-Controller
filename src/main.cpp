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

Display display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
Sensors sensors(ONE_WIRE_BUS);
RelayController relays;
Menu menu;
TimeManager timeManager;
ScheduleManager scheduler;

SensorData sensorData;
Statistics stats;
unsigned long lastReadTime = 0;
unsigned long lastStatsUpdate = 0;

void initStats() {
    stats.minTemp = 100.0; stats.maxTemp = -100.0;
    stats.minHum = 100.0; stats.maxHum = 0.0;
    stats.minLight = 10000.0; stats.maxLight = 0.0;
    stats.uptime = 0;
}

void updateStats(float temp, float hum, float light) {
    if (sensorData.ahtValid) {
        if (temp < stats.minTemp) stats.minTemp = temp;
        if (temp > stats.maxTemp) stats.maxTemp = temp;
        if (hum < stats.minHum) stats.minHum = hum;
        if (hum > stats.maxHum) stats.maxHum = hum;
    }
    if (sensorData.lightValid) {
        if (light < stats.minLight) stats.minLight = light;
        if (light > stats.maxLight) stats.maxLight = light;
    }
}

void setup() {
    Serial.begin(115200); delay(2000);
    logger.begin();
    logger.log("СИСТЕМА", "Запуск контроллера");

    Wire.begin(I2C_SDA, I2C_SCL);

    display.begin();
    sensors.begin();
    relays.begin();
    menu.begin();
    menu.setRelays(&relays);
    menu.setScheduler(&scheduler);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { delay(500); attempts++; }
    if (WiFi.status() == WL_CONNECTED) {
        logger.logf("Wi-Fi", "✅ Подключен, IP: %s", WiFi.localIP().toString().c_str());
        timeManager.begin();
    }

    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), Menu::readEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), Menu::readEncoderISR, CHANGE);

    scheduler.begin();
    scheduler.setHumidifierCyclic(HUMIDIFIER_WORK_TIME, HUMIDIFIER_IDLE_TIME);

    initStats();
    logger.log("СИСТЕМА", "✅ Запуск завершен");
    delay(2000);
}

void loop() {
    unsigned long now = millis();
    timeManager.update();

    if (now - lastReadTime > READ_INTERVAL) {
        sensorData = sensors.read();

        if (logger.shouldLog()) {
            if (timeManager.isTimeSynced())
                logger.logTime(timeManager.getHour(), timeManager.getMinute(), timeManager.getSecond());
            if (sensorData.ahtValid)
                logger.logSensors(sensorData.ahtTemp, sensorData.ahtHum, sensorData.dsTemp, sensorData.lightLevel);
            RelayState r = relays.getAllStates();
            logger.logRelay(r.lamp1, r.lamp2, r.lamp3, r.humidifier, menu.getControlMode());
        }

        if (menu.getControlMode() == MODE_AUTO && timeManager.isTimeSynced()) {
            relays.update(sensorData.lightLevel, sensorData.ahtHum,
                          timeManager.getHour(), timeManager.getMinute(), timeManager.getDayOfWeek());
        }

        if (menu.forceAutoUpdate()) {
            relays.update(sensorData.lightLevel, sensorData.ahtHum,
                          timeManager.getHour(), timeManager.getMinute(), timeManager.getDayOfWeek());
            menu.clearForceAutoUpdate();
        }

        lastReadTime = now;
    }

    if (menu.isToggleRequested()) {
        relays.manualToggle(menu.getToggleIndex());
        menu.clearToggleRequest();
    }

    if (Serial.available()) relays.handleSerialCommand(Serial.read());

    if (now - lastStatsUpdate > 60000) {
        stats.uptime = millis() / 1000;
        if (sensorData.ahtValid) updateStats(sensorData.ahtTemp, sensorData.ahtHum, sensorData.lightLevel);
        lastStatsUpdate = now;
    }

    RelayState rState = relays.getAllStates();
    menu.update(sensorData.ahtTemp, sensorData.ahtHum, sensorData.dsTemp, sensorData.lightLevel, rState, stats);
    delay(10);
}