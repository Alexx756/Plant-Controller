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
    float lightLevel;
    bool ahtValid;
    bool dsValid;
    bool lightValid;
};

class Sensors {
private:
    Adafruit_AHTX0 _aht;
    OneWire _oneWire;
    DallasTemperature _ds18b20;
    BH1750 _lightMeter;
    bool _ahtOK, _dsOK, _lightOK;
public:
    Sensors(int oneWirePin);
    bool begin();
    SensorData read();
};

#endif