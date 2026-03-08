#include "sensors.h"
#include "config.h"
#include <Arduino.h>

Sensors::Sensors(int oneWirePin) : _oneWire(oneWirePin), _ds18b20(&_oneWire) {
    _ahtOK = false;
    _dsOK = false;
    _lightOK = false;
}

bool Sensors::begin() {
    // AHT20
    _ahtOK = _aht.begin();
    
    // DS18B20
    _ds18b20.begin();
    _dsOK = (_ds18b20.getDeviceCount() > 0);
    
    // BH1750
    // В библиотеке Rob Tillaart begin() принимает адрес как второй параметр
    // и режим как первый. Режим можно не указывать — по умолчанию CONTINUOUS_HIGH_RES_MODE
    _lightOK = _lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR);
    
    return _ahtOK || _dsOK || _lightOK;
}

SensorData Sensors::read() {
    SensorData data;
    data.ahtValid = false;
    data.dsValid = false;
    data.lightValid = false;
    
    // AHT20
    if (_ahtOK) {
        sensors_event_t humidity, temp;
        if (_aht.getEvent(&humidity, &temp)) {
            data.ahtTemp = temp.temperature;
            data.ahtHum = humidity.relative_humidity;
            data.ahtValid = true;
        }
    }
    
    // DS18B20
    if (_dsOK) {
        _ds18b20.requestTemperatures();
        data.dsTemp = _ds18b20.getTempCByIndex(0);
        if (data.dsTemp > -50 && data.dsTemp < 125) {
            data.dsValid = true;
        }
    }
    
    // BH1750
    if (_lightOK) {
        // В библиотеке Rob Tillaart чтение делается через readLightLevel()
        data.lightLevel = _lightMeter.readLightLevel();
        data.lightValid = true;
    }
    
    return data;
}