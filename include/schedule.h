#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <Arduino.h>

// Forward declaration для ChannelSettings из relay.h
struct ChannelSettings;

// Структура для хранения расписания одного устройства
struct ScheduleItem {
    bool enabled;
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
    int days[7];  // массив дней (1-7), 0 означает "не выбран"
    int daysCount;
};

// Режимы работы увлажнителя
enum HumidifierMode {
    HUMIDIFIER_THRESHOLD,  // по порогу влажности
    HUMIDIFIER_CYCLIC,     // циклический (работа/отдых)
    HUMIDIFIER_SCHEDULE     // по расписанию
};

// Структура для настроек увлажнителя
struct HumidifierSettings {
    HumidifierMode mode;
    int threshold;          // порог влажности для HUMIDIFIER_THRESHOLD
    int workTime;           // секунд работы (для циклического)
    int idleTime;           // секунд отдыха (для циклического)
    ScheduleItem schedule;  // расписание для HUMIDIFIER_SCHEDULE
    bool lastState;         // последнее состояние (для циклического)
    unsigned long lastSwitch; // время последнего переключения
};

class ScheduleManager {
private:
    ScheduleItem _lamp1Schedule;
    ScheduleItem _lamp2Schedule;
    ScheduleItem _lamp3Schedule;
    HumidifierSettings _humidifier;
    
    bool checkSchedule(ScheduleItem& sched, int currentHour, int currentMin, int currentDay);
    
public:
    ScheduleManager();
    void begin();
    void update();  // вызывать в loop
    
    // Настройка расписания для ламп
    void setLampSchedule(int lamp, int startH, int startM, int endH, int endM, int* days, int daysCount);
    bool isLampScheduled(int lamp, int hour, int min, int day);
    
    // Настройка увлажнителя
    bool isHumidifierScheduled(int hour, int min, int day);
    void setHumidifierThreshold(int threshold);
    void setHumidifierCyclic(int workSec, int idleSec);
    void setHumidifierSchedule(int startH, int startM, int endH, int endM, int* days, int daysCount);
    bool shouldHumidifierRun(float currentHumidity);
    
    // Получение настроек
    HumidifierSettings getHumidifierSettings() { return _humidifier; }
    
    // Синхронизация с ChannelSettings из RelayController
    void syncLampFromSettings(int lampNumber, const ChannelSettings& settings);
    void syncHumidifierFromSettings(const ChannelSettings& settings);
};

// ============ РЕДАКТОР РАСПИСАНИЯ ЛАМП ============
class LampScheduleEditor {
private:
    int _lampNumber;                    // номер лампы (1-3)
    bool _useSensor;                    // использовать датчик
    bool _useSchedule;                  // использовать расписание
    int _threshold;                     // порог включения
    int _hysteresis;                    // гистерезис
    
    // Расписание
    bool _scheduleDays[7];              // дни недели (пн-вс)
    int _scheduleStartHour;             // время начала
    int _scheduleStartMin;
    int _scheduleEndHour;               // время конца
    int _scheduleEndMin;
    
    // Режим редактора: 0=дни, 1=время начала, 2=время конца
    int _editMode;
    // Активное редактируемое поле: -1 = нет, 0 = часы, 1 = минуты
    int _activeField;                   // активно ли редактирование времени
    int _editHour;                      // редактируемый час
    int _editMinute;                    // редактируемая минута
    
    ScheduleManager* _schedManager;     // указатель на планировщик
    
public:
    LampScheduleEditor();
    void begin(ScheduleManager* sched);
    
    // Начало редактирования (загрузка настроек лампы)
    void startEditing(int lampNumber, bool useSensor, bool useSchedule, 
                      int threshold, int hysteresis,
                      const bool days[7], int startHour, int startMin, 
                      int endHour, int endMin);
    
    // Навигация
    void navigate(int delta);
    void setEditMode(int mode);  // установка режима редактирования (0=дни, 1=начало, 2=конец)
    void switchEditMode();  // переключение между режимами редактирования
    void toggleDay(int dayIndex);  // переключение дня недели
    
    // Редактирование времени
    void startEditingTime();  // вход в режим редактирования времени (часы)
    void toggleTimeUnit();    // переключение между часами/минутами
    void saveAndExit();       // сохранение и выход из режима редактирования
    void editTime(int delta);
    void toggleTimeMode();  // переключение между часами/минутами (устарело, оставить для совместимости)
    
    // Изменение значений
    void toggleUseSensor();
    void toggleUseSchedule();
    void changeThreshold(int delta);
    void changeHysteresis(int delta);
    
    // Сохранение
    void saveToSchedule();
    
    // Геттеры для отрисовки
    int getLampNumber() const { return _lampNumber; }
    bool getUseSensor() const { return _useSensor; }
    bool getUseSchedule() const { return _useSchedule; }
    int getThreshold() const { return _threshold; }
    int getHysteresis() const { return _hysteresis; }
    bool getScheduleDay(int day) const { return _scheduleDays[day]; }
    int getScheduleStartHour() const { return _scheduleStartHour; }
    int getScheduleStartMin() const { return _scheduleStartMin; }
    int getScheduleEndHour() const { return _scheduleEndHour; }
    int getScheduleEndMin() const { return _scheduleEndMin; }
    int getEditMode() const { return _editMode; }
    int getActiveField() const { return _activeField; }
    int getEditHour() const { return _editHour; }
    int getEditMinute() const { return _editMinute; }
    bool isEditing() const { return _activeField >= 0; }
};

#endif