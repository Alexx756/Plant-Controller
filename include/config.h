#ifndef CONFIG_H
#define CONFIG_H

// ============ ПИНЫ ============
#define I2C_SDA 8
#define I2C_SCL 9
#define ONE_WIRE_BUS 2

#define RELAY_LAMP1 4
#define RELAY_LAMP2 5
#define RELAY_LAMP3 6
#define RELAY_HUMIDIFIER 7

#define ENCODER_CLK 10
#define ENCODER_DT 11
#define ENCODER_SW 12

// ============ АДРЕСА I2C ============
#define OLED_ADDR 0x3C
#define AHT20_ADDR 0x38
#define BH1750_ADDR 0x23

// ============ ПАРАМЕТРЫ АВТОМАТИКИ ============
#define LIGHT_THRESHOLD 200
#define HUMIDITY_THRESHOLD 40
#define TEMP_THRESHOLD 30

// ============ НАСТРОЙКИ УВЛАЖНИТЕЛЯ ============
#define HUMIDIFIER_WORK_TIME 30
#define HUMIDIFIER_IDLE_TIME 300

// ============ ПАРАМЕТРЫ ЦИКЛОВ ============
#define READ_INTERVAL 2000
#define DISPLAY_INTERVAL 1000
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ============ РЕЖИМЫ РАБОТЫ КАНАЛА ============
enum ChannelMode {
    CH_MODE_OFF,
    CH_MODE_ON,
    CH_MODE_SENSOR,
    CH_MODE_SCHEDULE,
    CH_MODE_SENSOR_SCHEDULE,
    CH_MODE_GLOBAL
};

enum ChannelType {
    CH_TYPE_LAMP,
    CH_TYPE_HUMIDIFIER
};

// ============ НАСТРОЙКИ КАНАЛА ============
struct ChannelSettings {
    ChannelMode mode;
    ChannelType type;
    int thresholdLow;
    int thresholdHigh;
    int sensorHysteresis;
    bool scheduleDays[7];
    int scheduleStartHour;
    int scheduleStartMin;
    int scheduleEndHour;
    int scheduleEndMin;
    int cycleWorkTime;
    int cycleIdleTime;
    bool cycleEnabled;
    bool cycleLastState;
    unsigned long cycleLastSwitch;
};

// ============ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ============
struct GlobalSettings {
    int globalLightThreshold;
    int globalLightHysteresis;
    int globalHumidityThreshold;
    int globalHumidityHysteresis;
    bool globalScheduleEnabled;
    bool globalScheduleDays[7];
    int globalScheduleStartHour;
    int globalScheduleStartMin;
    int globalScheduleEndHour;
    int globalScheduleEndMin;
};

// ============ НАСТРОЙКИ ОТЛАДКИ ============
#define LOG_ENABLED true
#define LOG_INTERVAL 5000
#define LOG_SENSORS true
#define LOG_RELAY true
#define LOG_ENCODER false
#define LOG_MENU false
#define LOG_AUTO true
#define LOG_TIME true
#define LOG_SCHEDULE false
#define LOG_DETAILED false

// ============ WI-FI НАСТРОЙКИ ============
#define WIFI_SSID "OpenWrt"
#define WIFI_PASSWORD "jkljkljkl!123"

// ============ НАСТРОЙКИ ВРЕМЕНИ ============
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 10800
#define DAYLIGHT_OFFSET_SEC 0

#endif


