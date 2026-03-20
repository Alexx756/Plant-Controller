#ifndef CONFIG_H
#define CONFIG_H

// ============ ПИНЫ I2C ============
#define I2C_SDA 8
#define I2C_SCL 9

// ============ АДРЕСА I2C ============
#define OLED_ADDR 0x3C
#define AHT20_ADDR 0x38
#define BH1750_ADDR 0x23

// ============ ПИНЫ ДАТЧИКОВ ============
#define ONE_WIRE_BUS 2

// ============ ПИНЫ РЕЛЕ ============
#define RELAY_LAMP1 4
#define RELAY_LAMP2 5
#define RELAY_LAMP3 6
#define RELAY_HUMIDIFIER 7

// ============ ПИНЫ ЭНКОДЕРА ============
#define ENCODER_CLK 10
#define ENCODER_DT 11
#define ENCODER_SW 12

// ============ НАСТРОЙКИ ВРЕМЕНИ ============
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 10800  // 3 * 3600 для Москвы (UTC+3)
#define DAYLIGHT_OFFSET_SEC 0 // 3600 если есть переход на летнее время

// ============ WI-FI НАСТРОЙКИ ============
#define WIFI_SSID "OpenWrt"
#define WIFI_PASSWORD "jkljkljkl!123"

// ============ ПАРАМЕТРЫ АВТОМАТИКИ ============
#define LIGHT_THRESHOLD 200
#define HUMIDITY_THRESHOLD 40
#define TEMP_THRESHOLD 30

// ============ НАСТРОЙКИ РЕЛЕ ПО УМОЛЧАНИЮ ============
// Лампы (секции 1,2,3)
#define LAMP1_ENABLED true
#define LAMP2_ENABLED true
#define LAMP3_ENABLED true

// Режимы работы увлажнителя
#define HUMIDIFIER_MODE_CYCLIC true      // true - циклический режим, false - по порогу
#define HUMIDIFIER_WORK_TIME 30           // секунд работает
#define HUMIDIFIER_IDLE_TIME 300          // секунд отдыхает (5 минут)

// ============ РЕЖИМЫ РАБОТЫ КАНАЛА ============
enum ChannelMode {
    CH_MODE_OFF,           // всегда выключен
    CH_MODE_ON,            // всегда включен
    CH_MODE_SENSOR,        // только по датчику
    CH_MODE_SCHEDULE,      // только по расписанию
    CH_MODE_SENSOR_SCHEDULE, // датчик + расписание (И)
    CH_MODE_GLOBAL         // использовать глобальные настройки
};

// ============ ТИПЫ КАНАЛОВ ============
enum ChannelType {
    CH_TYPE_LAMP,          // лампа (освещенность)
    CH_TYPE_HUMIDIFIER     // увлажнитель (влажность)
};

// ============ НАСТРОЙКИ КАНАЛА ============
struct ChannelSettings {
    // Общие настройки
    int index;  // 1-лампа1, 2-лампа2, 3-лампа3, 4-увлажнитель
    ChannelType type; 
    bool useSensor;      // использовать датчик
    bool useCyclic;      // использовать циклический режим
    bool useSchedule;    // использовать расписание

    ChannelMode mode;
    bool enabled;
    
    // Настройки датчика
    int thresholdLow;       // порог включения
    int thresholdHigh;      // порог выключения (для гистерезиса)
    int sensorHysteresis;   // гистерезис (разница между вкл/выкл)
    
    // Настройки расписания
    bool scheduleDays[7];   // дни недели [0=пн, 1=вт, 2=ср, 3=чт, 4=пт, 5=сб, 6=вс]
    int scheduleStartHour;
    int scheduleStartMin;
    int scheduleEndHour;
    int scheduleEndMin;
    
    // Для циклических устройств (увлажнитель)
    int cycleWorkTime;       // секунд работы
    int cycleIdleTime;       // секунд отдыха
    bool cycleEnabled;       // использовать циклический режим (вместо порога)
    
    // Состояние для циклического режима
    bool cycleLastState;
    unsigned long cycleLastSwitch;
};

// ============ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ============
struct GlobalSettings {
    ChannelSettings lamp1Default;
    ChannelSettings lamp2Default;
    ChannelSettings lamp3Default;
    ChannelSettings humidifierDefault;
    
    // Глобальное расписание (для режима GLOBAL)
    bool globalScheduleEnabled;
    bool globalScheduleDays[7];
    int globalScheduleStartHour;
    int globalScheduleStartMin;
    int globalScheduleEndHour;
    int globalScheduleEndMin;
    
    // Глобальные пороги (для режима GLOBAL)
    int globalLightThreshold;
    int globalLightHysteresis;
    int globalHumidityThreshold;
    int globalHumidityHysteresis;
};

// ============ ПАРАМЕТРЫ ЦИКЛОВ ============
#define READ_INTERVAL 2000        // чтение датчиков раз в 2 секунды
#define DISPLAY_INTERVAL 1000     // обновление дисплея раз в секунду

// ============ НАСТРОЙКИ ЛОГИРОВАНИЯ ============
#define LOG_ENABLED true           // включить/выключить все логи
#define LOG_INTERVAL 5000          // интервал вывода (мс)

// Категории логов (какие данные выводить)
#define LOG_SENSORS true           // датчики (температура, влажность, свет)
#define LOG_RELAY true             // состояние реле
#define LOG_ENCODER false          // энкодер (отладка вращения)
#define LOG_MENU false             // переходы по меню
#define LOG_AUTO true              // автоматика (включение/выключение)
#define LOG_TIME true              // время (если есть NTP/RTC)
#define LOG_SCHEDULE true
#define LOG_DEBUG true             // отладочные сообщения (разное)


// Формат вывода
#define LOG_DETAILED false          // подробный режим

// ============ НАСТРОЙКИ ОТЛАДКИ ============
#define SERIAL_DEBUG true        // true - вывод в Serial, false - отключен
#define SERIAL_INTERVAL 10000      // вывод в Serial раз в 10 секунд (если включен)
#define SERIAL_DETAILED false      // подробный вывод (все датчики) или краткий

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#endif