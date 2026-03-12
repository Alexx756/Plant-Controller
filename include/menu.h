#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "config.h"
#include "relay.h"
#include "schedule.h"  

// Типы экранов
enum ScreenType {
    SCREEN_MAIN,                    // Главный экран с датчиками
    SCREEN_MODE_SELECT,              // Выбор режима (AUTO/MANUAL/SETTINGS)
    SCREEN_MANUAL,                   // Ручное управление реле
    SCREEN_SETTINGS,                  // Меню настроек
    SCREEN_STATS,                     // Статистика
    SCREEN_CONFIRM,                    // Подтверждение действий
    SCREEN_HUMIDIFIER_SETTINGS,        // выбор режима увлажнителя
    SCREEN_HUMIDIFIER_THRESHOLD,       // настройка порога
    SCREEN_HUMIDIFIER_CYCLIC,          // настройка циклов
    SCREEN_HUMIDIFIER_SCHEDULE,        // настройка расписания
    SCREEN_LAMP_SETTINGS,              // настройки ламп (общее меню)
    SCREEN_LAMP1_SETTINGS,             // настройки лампы 1
    SCREEN_LAMP2_SETTINGS,             // настройки лампы 2
    SCREEN_LAMP3_SETTINGS,             // настройки лампы 3
    SCREEN_LAMP_SCHEDULE,              // настройка расписания ламп
    SCREEN_LAMP_DAYS,                  // выбор дней недели для ламп
};

// Режим управления
enum ControlMode {
    MODE_AUTO,      // Автоматический (реле управляются датчиками)
    MODE_MANUAL     // Ручной (реле управляются с энкодера)
};

// Структура для порогов
struct Thresholds {
    int lightThreshold;
    int humidityThreshold;
    float tempThreshold;
};

// Структура для статистики
struct Statistics {
    float minTemp;
    float maxTemp;
    float minHum;
    float maxHum;
    float minLight;
    float maxLight;
    unsigned long uptime;
};

class Menu {
private:
    // ===== ТЕКУЩИЙ ЭКРАН И НАВИГАЦИЯ =====
    ScreenType _currentScreen;      // текущий отображаемый экран
    ScreenType _prevScreen;          // предыдущий экран (для возврата)
    int _menuPosition;               // позиция курсора на текущем экране

    // ===== ЭНКОДЕР =====
    AiEsp32RotaryEncoder* _encoder;  // объект энкодера
    volatile long _encoderValue;      // последнее считанное значение (для ISR)
    bool _encoderPressed;             // флаг нажатия кнопки энкодера
    unsigned long _pressStartTime;    // время начала нажатия (для определения длительности)
    bool _longPressTriggered;          // флаг, что долгое нажатие уже обработано

    // ===== РЕЖИМЫ И НАСТРОЙКИ =====
    ControlMode _controlMode;          // AUTO или MANUAL
    Thresholds _thresholds;            // текущие пороги (свет, влажность, температура)
    Thresholds _editThresholds;        // редактируемые пороги (для экрана настроек)
    ScheduleManager* _scheduler;       // указатель на планировщик расписаний
    int _editingValue;                 // вспомогательная переменная для редактирования (используется в разных экранах)
    bool _forceAutoUpdate;              // флаг принудительного обновления AUTO режима

    // ===== РУЧНОЕ УПРАВЛЕНИЕ РЕЛЕ =====
    bool _manualToggleRequested;        // запрос на переключение реле из меню
    int _manualToggleIndex;             // индекс реле для переключения (0-3)

    // ===== НАСТРОЙКИ УВЛАЖНИТЕЛЯ =====
    int _editHumidifierMode;            // выбранный режим увлажнителя (0-2)
    int _editHumidifierValue;           // первое редактируемое значение (нижний порог / время работы)
    int _editHumidifierValue2;           // второе редактируемое значение (гистерезис / время отдыха)
    bool _editHumidifierSensorEnabled;   // для экрана порога (useSensor)
    bool _editHumidifierCyclicEnabled;   // для экрана цикла (useCyclic)
    
    // ===== НАСТРОЙКИ ЛАМП =====
    int _editLampNumber;                 // номер редактируемой лампы (1-3)
    bool _editLampUseSensor;             // использовать датчик света
    bool _editLampUseSchedule;           // использовать расписание
    int _editLampThreshold;              // порог включения (для датчика)
    int _editLampHysteresis;             // гистерезис
    // Расписание
    bool _editLampScheduleDays[7];       // дни недели
    int _editLampScheduleStartHour;      // время начала
    int _editLampScheduleStartMin;
    int _editLampScheduleEndHour;        // время конца
    int _editLampScheduleEndMin;
    
    // ===== РЕДАКТИРОВАНИЕ РАСПИСАНИЯ ЛАМП =====
    int _lampScheduleEditMode;           // 0=дни, 1=время начала, 2=время конца
    bool _lampEditTimeMode;              // false=часы, true=минуты (для времени)

    // ===== РЕДАКТИРОВАНИЕ ВРЕМЕНИ =====
    int _editHour, _editMinute;          // часы и минуты для редактирования времени
    bool _editIsHour;                     // флаг: сейчас редактируем часы (true) или минуты (false)

    // ===== УКАЗАТЕЛИ НА ДРУГИЕ МОДУЛИ =====
    RelayController* _relay;             // указатель на контроллер реле

    // ===== НОВЫЕ ПЕРЕМЕННЫЕ ДЛЯ РЕЖИМА РЕДАКТИРОВАНИЯ =====
    bool _editingActive;                 // флаг: находимся в режиме редактирования значения
    int _editingIndex;                    // индекс редактируемого поля (1 или 2 на экране датчика)

    // ===== ВНУТРЕННИЕ МЕТОДЫ =====
    void handleEncoderRotation(int delta);   // обработка вращения энкодера
    void handleShortPress();                 // обработка короткого нажатия
    void handleLongPress();                  // обработка долгого нажатия

    // ===== ОТРИСОВКА ЭКРАНОВ =====
    void drawMainScreen(float temp, float hum, float dsTemp, float light, RelayState relayState);
    void drawModeSelectScreen();
    void drawManualScreen(RelayState relayState);
    void drawHumidifierSettingsScreen();
    void drawHumidifierThresholdScreen();
    void drawHumidifierCyclicScreen();
    void drawHumidifierScheduleScreen(); 
    void drawLampSettingsScreen();
    void drawLampDetailScreen(int lampNumber);
    void drawLampScheduleScreen(int lampNumber);
    void drawLampDaysScreen();
    void drawSettingsScreen();
    void drawStatsScreen(Statistics stats);
    void drawConfirmScreen(const char* message);

public:
    Menu();
    void begin();
    void update(float temp, float hum, float dsTemp, float light, RelayState relayState, Statistics stats);

    // Для отладки
    void testEncoder();

    // IRAM_ATTR для обработки прерывания
    static void IRAM_ATTR readEncoderISR();

    // Геттеры
    ScreenType getCurrentScreen() { return _currentScreen; }
    ControlMode getControlMode() { return _controlMode; }
    Thresholds getThresholds() { return _thresholds; }

    bool isToggleRequested() { return _manualToggleRequested; }
    int getToggleIndex() { return _manualToggleIndex; }
    void clearToggleRequest() { _manualToggleRequested = false; }

    bool forceAutoUpdate() { return _forceAutoUpdate; }
    void clearForceAutoUpdate() { _forceAutoUpdate = false; }

    // Сеттеры
    void setScheduler(ScheduleManager* sched) { _scheduler = sched; }
    void setControlMode(ControlMode mode) { _controlMode = mode; }
    void setThresholds(Thresholds th) { _thresholds = th; }
    void setRelayController(RelayController* relay) { _relay = relay; }
};

#endif