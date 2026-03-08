#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Adafruit_AHTX0.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h> 

struct SensorData {
    float ahtTemp;
    float ahtHum;
    float dsTemp;
    float lightLevel;         // ← добавили
    bool ahtValid;
    bool dsValid;
    bool lightValid;          // ← добавили
};

class Sensors {
private:
    Adafruit_AHTX0 _aht;
    OneWire _oneWire;
    DallasTemperature _ds18b20;
    BH1750 _lightMeter;       // ← добавили
    bool _ahtOK;
    bool _dsOK;
    bool _lightOK;            // ← добавили
    
public:
    Sensors(int oneWirePin);
    bool begin();
    SensorData read();
};

#endif