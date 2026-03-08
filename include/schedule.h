#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <Arduino.h>

enum HumidifierMode {
    HUMIDIFIER_THRESHOLD,
    HUMIDIFIER_CYCLIC,
    HUMIDIFIER_SCHEDULE
};

struct HumidifierSettings {
    HumidifierMode mode;
    int threshold;
    int workTime;
    int idleTime;
    bool lastState;
    unsigned long lastSwitch;
    int scheduleStartHour;
    int scheduleStartMin;
    int scheduleEndHour;
    int scheduleEndMin;
};

class ScheduleManager {
private:
    HumidifierSettings _humidifier;
public:
    ScheduleManager();
    void begin();
    void update();
    void setHumidifierThreshold(int threshold);
    void setHumidifierCyclic(int workSec, int idleSec);
    bool shouldHumidifierRun(float humidity);
    HumidifierSettings getHumidifierSettings() { return _humidifier; }
};

#endif