#include "sensors.h"
#include "config.h"
#include <Wire.h>

Sensors::Sensors(int oneWirePin) : _oneWire(oneWirePin), _ds18b20(&_oneWire) {
    _ahtOK = _dsOK = _lightOK = false;
}

bool Sensors::begin() {
    _ahtOK = _aht.begin();
    _ds18b20.begin();
    _dsOK = (_ds18b20.getDeviceCount() > 0);
    // Используем begin без указания режима – по умолчанию CONTINUOUS_HIGH_RES_MODE
    _lightOK = _lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR);
    
    return _ahtOK || _dsOK || _lightOK;
}
SensorData Sensors::read() {
    SensorData data;
    data.ahtValid = data.dsValid = data.lightValid = false;

    if (_ahtOK) {
        sensors_event_t hum, temp;
        if (_aht.getEvent(&hum, &temp)) {
            data.ahtTemp = temp.temperature;
            data.ahtHum = hum.relative_humidity;
            data.ahtValid = true;
        }
    }
    if (_dsOK) {
        _ds18b20.requestTemperatures();
        data.dsTemp = _ds18b20.getTempCByIndex(0);
        if (data.dsTemp > -50 && data.dsTemp < 125) data.dsValid = true;
    }
    if (_lightOK) {
        data.lightLevel = _lightMeter.readLightLevel();
        data.lightValid = true;
    }
    return data;
}