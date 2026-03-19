#include "menu.h"
#include "display.h"
#include <Arduino.h>
#include "time_manager.h"
#include "logger.h"

extern Display display;

// Глобальный указатель для ISR
Menu* menuInstance = nullptr;

Menu::Menu() {
    _currentScreen = SCREEN_MAIN;
    _prevScreen = SCREEN_MAIN;
    _menuPosition = 0;
    
    // Энкодер
    _encoder = nullptr;
    _encoderPressed = false;
    _pressStartTime = 0;
    _longPressTriggered = false;
    
    // Планировщик
    _scheduler = nullptr;
    _lampEditor = nullptr;
    
    // Флаг принудительного обновления AUTO
    _forceAutoUpdate = false;
    
    _controlMode = MODE_AUTO;
    
    _thresholds.lightThreshold = LIGHT_THRESHOLD;
    _thresholds.humidityThreshold = HUMIDITY_THRESHOLD;
    _thresholds.tempThreshold = TEMP_THRESHOLD;
    
    _editHumidifierMode = 0;
    _editHumidifierValue = 0;
    _editHumidifierValue2 = 0;
    _editHumidifierSensorEnabled = false;
    _editHumidifierCyclicEnabled = false;
    
    _relay = nullptr;
    
    _manualToggleRequested = false;
    _manualToggleIndex = 0;

    // Новые поля для режима редактирования
    _editingActive = false;
    _editingIndex = 0;
    
    // Номер текущей лампы
    _currentLampNumber = 0;
    
    menuInstance = this;
}

void Menu::begin() {
    Serial.println("🔄 Инициализация энкодера...");
    
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    _encoder = new AiEsp32RotaryEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, -1, 4);
    _encoder->begin();
    _encoder->setup(readEncoderISR);
    
    // Настройка ускорения энкодера
 
    _encoder->setAcceleration(150);

    
    Serial.println("✅ Энкодер инициализирован с ускорением");
    
    // Для отладки можно раскомментировать, но в рабочей версии лучше убрать
    // testEncoder();
}

// ISR функция
void IRAM_ATTR Menu::readEncoderISR() {
    if (menuInstance && menuInstance->_encoder) {
        menuInstance->_encoder->readEncoder_ISR();
    }
}

void Menu::update(float temp, float hum, float dsTemp, float light, RelayState relayState, Statistics stats) {
    if (!_encoder) return;
    
    static long lastValue = 0;
    long currentValue = _encoder->readEncoder();
    
    if (currentValue != lastValue) {
        // Вычисляем реальную дельту с учетом ускорения
        int delta = (int)(currentValue - lastValue);
        // Ограничиваем дельту для безопасности (максимум 10 за один вызов)
        if (delta > 10) delta = 10;
        if (delta < -10) delta = -10;
        
        if (delta != 0) {
            logger.logEncoder(delta, _menuPosition);
            handleEncoderRotation(delta);
        }
        lastValue = currentValue;
    }
    
    // Обработка кнопки энкодера
    static unsigned long pressStartTime = 0;
    static bool longPressHandled = false;
    bool buttonDown = _encoder->isEncoderButtonDown();
    
    if (buttonDown) {
        if (pressStartTime == 0) {
            pressStartTime = millis();
            longPressHandled = false;
            logger.logEncoder(0, 999);
        } else if (!longPressHandled && (millis() - pressStartTime > 1000)) {
            logger.logEncoder(0, 998);
            handleLongPress();
            longPressHandled = true;
        }
    } else {
        if (pressStartTime != 0 && !longPressHandled) {
            logger.logEncoder(0, 997);
            handleShortPress();
        }
        pressStartTime = 0;
        longPressHandled = false;
    }
    
    // Отрисовка текущего экрана
    switch (_currentScreen) {
        case SCREEN_MAIN:
            drawMainScreen(temp, hum, dsTemp, light, relayState);
            break;
        case SCREEN_MODE_SELECT:
            drawModeSelectScreen();
            break;
        case SCREEN_MANUAL:
            drawManualScreen(relayState);
            break;
        case SCREEN_SETTINGS:
            drawSettingsScreen();
            break;
        case SCREEN_STATS:
            drawStatsScreen(stats);
            break;
        case SCREEN_HUMIDIFIER_SETTINGS:
            drawHumidifierSettingsScreen();
            break;
        case SCREEN_HUMIDIFIER_THRESHOLD:
            drawHumidifierThresholdScreen();
            break;
        case SCREEN_HUMIDIFIER_CYCLIC:
            drawHumidifierCyclicScreen();
            break;
        case SCREEN_HUMIDIFIER_SCHEDULE:
            drawHumidifierScheduleScreen();
            break;
        case SCREEN_LAMP_SETTINGS:
            drawLampSettingsScreen();
            break;
        case SCREEN_LAMP1_SETTINGS:
        case SCREEN_LAMP2_SETTINGS:
        case SCREEN_LAMP3_SETTINGS:
            drawLampDetailScreen(_currentLampNumber);
            break;
        case SCREEN_LAMP_SCHEDULE:
            drawLampScheduleScreen(_currentLampNumber);
            break;
        case SCREEN_LAMP_DAYS:
            drawLampDaysScreen();
            break;
        default:
            break;
    }
}

void Menu::handleEncoderRotation(int delta) {
    logger.logEncoder(delta, _menuPosition);
    
    switch (_currentScreen) {
        case SCREEN_MODE_SELECT:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
            break;
            
        case SCREEN_MANUAL:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 3;
            if (_menuPosition > 3) _menuPosition = 0;
            break;
            
        case SCREEN_SETTINGS:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 3;
            if (_menuPosition > 3) _menuPosition = 0;
            break;
            
        case SCREEN_HUMIDIFIER_SETTINGS:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
            break;
            
        case SCREEN_HUMIDIFIER_THRESHOLD:
            if (_editingActive) {
                // Режим редактирования: меняем значение в зависимости от _editingIndex
                if (_editingIndex == 1) {
                    _editHumidifierValue += delta * 5;
                    if (_editHumidifierValue < 0) _editHumidifierValue = 0;
                    if (_editHumidifierValue > 100) _editHumidifierValue = 100;
                } else if (_editingIndex == 2) {
                    _editHumidifierValue2 += delta * 5;
                    if (_editHumidifierValue2 < 0) _editHumidifierValue2 = 0;
                    if (_editHumidifierValue2 > 100) _editHumidifierValue2 = 100;
                }
            } else {
                // Навигация: перемещаем курсор между пунктами 0,1,2
                _menuPosition += delta;
                if (_menuPosition < 0) _menuPosition = 2;
                if (_menuPosition > 2) _menuPosition = 0;
            }
            break;
            
        case SCREEN_HUMIDIFIER_CYCLIC:
            if (_editingActive) {
                if (_editingIndex == 1) {
                    _editHumidifierValue += delta * 10;
                    if (_editHumidifierValue < 10) _editHumidifierValue = 10;
                    if (_editHumidifierValue > 600) _editHumidifierValue = 600;
                } else if (_editingIndex == 2) {
                    _editHumidifierValue2 += delta * 10;
                    if (_editHumidifierValue2 < 10) _editHumidifierValue2 = 10;
                    if (_editHumidifierValue2 > 600) _editHumidifierValue2 = 600;
                }
            } else {
                _menuPosition += delta;
                if (_menuPosition < 0) _menuPosition = 2;
                if (_menuPosition > 2) _menuPosition = 0;
            }
            break;
            
        case SCREEN_LAMP_SETTINGS:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
            break;
            
        case SCREEN_LAMP1_SETTINGS:
        case SCREEN_LAMP2_SETTINGS:
        case SCREEN_LAMP3_SETTINGS:
            if (_editingActive && (_menuPosition == 1 || _menuPosition == 2)) {
                // Режим редактирования: меняем значение порога или гистерезиса
                if (_menuPosition == 1) { // Порог
                    _editHumidifierValue += delta * 10;
                    if (_editHumidifierValue < 0) _editHumidifierValue = 0;
                    if (_editHumidifierValue > 1000) _editHumidifierValue = 1000;
                } else if (_menuPosition == 2) { // Гистерезис
                    _editHumidifierValue2 += delta * 10;
                    if (_editHumidifierValue2 < 0) _editHumidifierValue2 = 0;
                    if (_editHumidifierValue2 > 100) _editHumidifierValue2 = 100;
                }
            } else {
                // Навигация по меню
                _menuPosition += delta;
                if (_menuPosition < 0) _menuPosition = 3;
                if (_menuPosition > 3) _menuPosition = 0;
            }
            break;
            
        case SCREEN_LAMP_SCHEDULE:
            // Если редактор в режиме редактирования времени, передаём навигацию
            if (_lampEditor && _lampEditor->isEditing()) {
                _lampEditor->navigate(delta);
            } else {
                // Иначе навигация по меню (0-дни, 1-начало, 2-конец)
                _menuPosition += delta;
                if (_menuPosition < 0) _menuPosition = 2;
                if (_menuPosition > 2) _menuPosition = 0;
            }
            break;
            
        case SCREEN_LAMP_DAYS:
            // Прямая навигация по дням недели (0-6) для выбора текущего дня
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 6;
            if (_menuPosition > 6) _menuPosition = 0;
            break;
            
        default:
            break;
    }
}

void Menu::handleShortPress() {
    logger.logMenu("Нажатие", _currentScreen);
    
    switch (_currentScreen) {
        case SCREEN_MAIN:
            _currentScreen = SCREEN_MODE_SELECT;
            _menuPosition = 0;
            logger.logMenu("→ Выбор режима", _currentScreen);
            break;
            
        case SCREEN_MODE_SELECT:
            switch (_menuPosition) {
                case 0: // AUTO
                    _controlMode = MODE_AUTO;
                    _currentScreen = SCREEN_MAIN;
                    _forceAutoUpdate = true;
                    logger.logMenu("→ AUTO режим", _currentScreen);
                    break;
                case 1: // MANUAL
                    _controlMode = MODE_MANUAL;
                    _currentScreen = SCREEN_MANUAL;
                    _menuPosition = 0;
                    logger.logMenu("→ MANUAL режим", _currentScreen);
                    break;
                case 2: // НАСТРОЙКИ
                    _currentScreen = SCREEN_SETTINGS;
                    _menuPosition = 0;
                    logger.logMenu("→ Настройки", _currentScreen);
                    break;
            }
            break;
            
        case SCREEN_MANUAL:
            Serial.printf("Ручное переключение реле %d\n", _menuPosition);
            _manualToggleRequested = true;
            _manualToggleIndex = _menuPosition;
            break;
            
        case SCREEN_SETTINGS:
            switch (_menuPosition) {
                case 0: // Лампы
                    _currentScreen = SCREEN_LAMP_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 1: // Увлажнитель
                    _currentScreen = SCREEN_HUMIDIFIER_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 2: // Статистика
                    _currentScreen = SCREEN_STATS;
                    _menuPosition = 0;
                    break;
            }
            break;
            
        case SCREEN_STATS:
            _currentScreen = SCREEN_SETTINGS;
            break;
            
        case SCREEN_HUMIDIFIER_SETTINGS:
            switch (_menuPosition) {
                case 0: // Датчик (по порогу)
                    if (_relay) {
                        auto* s = _relay->getHumidifierSettings();
                        _editHumidifierSensorEnabled = s->useSensor;
                        _editHumidifierValue = s->thresholdLow;
                        _editHumidifierValue2 = s->sensorHysteresis; // гистерезис
                    }
                    _currentScreen = SCREEN_HUMIDIFIER_THRESHOLD;
                    _menuPosition = 0;
                    _editingActive = false;
                    logger.logMenu("→ Настройка датчика", _currentScreen);
                    break;
                    
                case 1: // Циклический
                    if (_relay) {
                        auto* s = _relay->getHumidifierSettings();
                        _editHumidifierCyclicEnabled = s->useCyclic;
                        _editHumidifierValue = s->cycleWorkTime;
                        _editHumidifierValue2 = s->cycleIdleTime;
                    }
                    _currentScreen = SCREEN_HUMIDIFIER_CYCLIC;
                    _menuPosition = 0;
                    _editingActive = false;
                    logger.logMenu("→ Настройка цикла", _currentScreen);
                    break;
                    
                case 2: // Расписание
                    _currentScreen = SCREEN_HUMIDIFIER_SCHEDULE;
                    _menuPosition = 0;
                    logger.logMenu("→ Настройка расписания", _currentScreen);
                    break;
            }
            break;
            
        case SCREEN_HUMIDIFIER_THRESHOLD:
            if (_editingActive) {
                // Выходим из режима редактирования с сохранением
                if (_relay) {
                    auto* s = _relay->getHumidifierSettings();
                    if (_editingIndex == 1) {
                        s->thresholdLow = _editHumidifierValue;
                        s->thresholdHigh = s->thresholdLow + s->sensorHysteresis;
                    } else if (_editingIndex == 2) {
                        s->sensorHysteresis = _editHumidifierValue2;
                        s->thresholdHigh = s->thresholdLow + s->sensorHysteresis;
                    }
                }
                _editingActive = false;
            } else {
                if (_menuPosition == 0) {
                    _editHumidifierSensorEnabled = !_editHumidifierSensorEnabled;
                    if (_relay) _relay->setHumidifierUseSensor(_editHumidifierSensorEnabled);
                } else {
                    // Входим в режим редактирования
                    _editingActive = true;
                    _editingIndex = _menuPosition;
                }
            }
            break;
            
        case SCREEN_HUMIDIFIER_CYCLIC:
            if (_menuPosition == 0) {
                _editHumidifierCyclicEnabled = !_editHumidifierCyclicEnabled;
                if (_relay) {
                    _relay->setHumidifierUseCyclic(_editHumidifierCyclicEnabled);
                }
            } else {
                if (!_editingActive) {
                    _editingActive = true;
                    _editingIndex = _menuPosition;
                } else {
                    _editingActive = false;
                    if (_relay) {
                        if (_editingIndex == 1 || _editingIndex == 2) {
                            // Сохраняем оба значения при выходе из редактирования любого
                            _relay->setHumidifierCycleTimes(_editHumidifierValue, _editHumidifierValue2);
                        }
                    }
                }
            }
            break;
            
        case SCREEN_LAMP_SETTINGS:
            switch (_menuPosition) {
                case 0:
                    _currentLampNumber = 1;
                    _currentScreen = SCREEN_LAMP1_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 1:
                    _currentLampNumber = 2;
                    _currentScreen = SCREEN_LAMP2_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 2:
                    _currentLampNumber = 3;
                    _currentScreen = SCREEN_LAMP3_SETTINGS;
                    _menuPosition = 0;
                    break;
            }
            break;
            
        case SCREEN_LAMP1_SETTINGS:
        case SCREEN_LAMP2_SETTINGS:
        case SCREEN_LAMP3_SETTINGS:
            // Если в режиме редактирования порога/гистерезиса - сохраняем и выходим из режима
            if (_editingActive && (_menuPosition == 1 || _menuPosition == 2)) {
                if (_relay) {
                    int lampIndex = _currentLampNumber - 1;
                    if (_editingIndex == 1) { // Порог
                        _relay->setLampThreshold(_currentLampNumber, _editHumidifierValue, _editHumidifierValue2);
                    } else if (_editingIndex == 2) { // Гистерезис
                        _relay->setLampHysteresis(_currentLampNumber, _editHumidifierValue2);
                    }
                }
                _editingActive = false;
                // Остаемся на том же экране, выходим из switch
                break;
            }
            
            if (_menuPosition == 3) { // Расписание
                _currentScreen = SCREEN_LAMP_SCHEDULE;
                _menuPosition = 0;
                
                // Используем LampScheduleEditor
                if (_lampEditor && _relay) {
                    ChannelSettings* s = nullptr;
                    if (_currentLampNumber == 1) s = _relay->getLamp1Settings();
                    else if (_currentLampNumber == 2) s = _relay->getLamp2Settings();
                    else if (_currentLampNumber == 3) s = _relay->getLamp3Settings();
                    
                    if (s) {
                        _lampEditor->startEditing(
                            _currentLampNumber,
                            s->useSensor, s->useSchedule,
                            s->thresholdLow, s->sensorHysteresis,
                            s->scheduleDays,
                            s->scheduleStartHour, s->scheduleStartMin,
                            s->scheduleEndHour, s->scheduleEndMin
                        );
                    }
                }
            } else {
                // Обработка нажатий в детальном экране лампы
                if (_relay) {
                    ChannelSettings* s = nullptr;
                    if (_currentLampNumber == 1) s = _relay->getLamp1Settings();
                    else if (_currentLampNumber == 2) s = _relay->getLamp2Settings();
                    else if (_currentLampNumber == 3) s = _relay->getLamp3Settings();
                    
                    if (s) {
                        if (_menuPosition == 0) { // Включить/выключить датчик
                            bool currentState = s->useSensor;
                            _relay->setLampUseSensor(_currentLampNumber, !currentState);
                        } else if (_menuPosition == 1) { // Порог
                            // Загружаем текущие значения
                            _editHumidifierValue = s->thresholdLow;
                            _editHumidifierValue2 = s->sensorHysteresis;
                            // Входим в режим редактирования
                            _editingActive = true;
                            _editingIndex = _menuPosition;
                        } else if (_menuPosition == 2) { // Гистерезис
                            // Загружаем текущие значения
                            _editHumidifierValue = s->thresholdLow;
                            _editHumidifierValue2 = s->sensorHysteresis;
                            // Входим в режим редактирования
                            _editingActive = true;
                            _editingIndex = _menuPosition;
                        }
                    }
                }
            }
            break;
            
        case SCREEN_LAMP_SCHEDULE:
            // Используем LampScheduleEditor
            if (_lampEditor) {
                if (_menuPosition == 0) {
                    // Переход к экрану выбора дней недели
                    _currentScreen = SCREEN_LAMP_DAYS;
                    _menuPosition = 0;
                    break;
                } else if (_menuPosition == 1 || _menuPosition == 2) {
                    // Редактирование времени
                    if (!_lampEditor->isEditing()) {
                        // Первое нажатие: активировать редактирование часов
                        _lampEditor->setEditMode(_menuPosition == 1 ? 1 : 2);
                        _lampEditor->startEditingTime();
                        // _activeField = 0 (часы) устанавливается в startEditingTime()
                    } else {
                        // Уже в режиме редактирования
                        int activeField = _lampEditor->getActiveField();
                        if (activeField == 0) {
                            // Были в часах, переключаем на минуты
                            _lampEditor->toggleTimeUnit();
                        } else if (activeField == 1) {
                            // Были в минутах, сохраняем и выходим
                            _lampEditor->saveAndExit();
                        }
                    }
                }
            }
            break;
            
        case SCREEN_LAMP_DAYS:
            // Управление через редактор - переключение дня
            if (_lampEditor) {
                if (_menuPosition >= 0 && _menuPosition <= 6) {
                    _lampEditor->toggleDay(_menuPosition);
                }
            }
            break;
            
        case SCREEN_CONFIRM:
            _currentScreen = _prevScreen;
            break;
            
        default:
            break;
    }
}

void Menu::handleLongPress() {
    // Длительное нажатие в SCREEN_LAMP_DAYS: сохраняем дни и возвращаемся в SCREEN_LAMP_SCHEDULE
    if (_currentScreen == SCREEN_LAMP_DAYS && _lampEditor) {
        _lampEditor->saveToSchedule();
        _currentScreen = SCREEN_LAMP_SCHEDULE;
        _menuPosition = 0;
        return;
    }

    // Сохраняем расписание ламп через редактор и возвращаемся в настройки лампы
    if (_currentScreen == SCREEN_LAMP_SCHEDULE && _lampEditor) {
        _lampEditor->saveToSchedule();
        if (_relay) {
            int lampNumber = _lampEditor->getLampNumber();
            _relay->setLampUseSchedule(lampNumber, true);
        }
        // Возвращаемся в экран настроек соответствующей лампы (пункт "Расписание")
        _currentScreen = static_cast<ScreenType>(SCREEN_LAMP1_SETTINGS + _lampEditor->getLampNumber() - 1);
        _menuPosition = 3;
        return;
    }

    // Сохраняем текущее редактируемое значение, если в режиме
    if (_editingActive) {
        if (_relay) {
            if (_currentScreen == SCREEN_HUMIDIFIER_THRESHOLD) {
                auto* s = _relay->getHumidifierSettings();
                if (_editingIndex == 1) {
                    s->thresholdLow = _editHumidifierValue;
                    s->thresholdHigh = s->thresholdLow + s->sensorHysteresis;
                } else if (_editingIndex == 2) {
                    s->sensorHysteresis = _editHumidifierValue2;
                    s->thresholdHigh = s->thresholdLow + s->sensorHysteresis;
                }
            } else if (_currentScreen == SCREEN_HUMIDIFIER_CYCLIC) {
                if (_editingIndex == 1 || _editingIndex == 2) {
                    _relay->setHumidifierCycleTimes(_editHumidifierValue, _editHumidifierValue2);
                }
            } else if (_currentScreen == SCREEN_LAMP1_SETTINGS || _currentScreen == SCREEN_LAMP2_SETTINGS || _currentScreen == SCREEN_LAMP3_SETTINGS) {
                // Сохраняем настройки ламп через сеттеры
                if (_editingIndex == 1) { // Порог
                    _relay->setLampThreshold(_currentLampNumber, _editHumidifierValue, _editHumidifierValue2);
                } else if (_editingIndex == 2) { // Гистерезис
                    _relay->setLampHysteresis(_currentLampNumber, _editHumidifierValue2);
                }
            }
        }
        _editingActive = false;
    }
    _currentScreen = SCREEN_MAIN;
    _menuPosition = 0;
}

// ==================== ФУНКЦИИ ОТРИСОВКИ ====================

void Menu::drawMainScreen(float temp, float hum, float dsTemp, float light, RelayState relayState) {
    display.clear();
    
    extern TimeManager timeManager;
    String timeStr = timeManager.getFormattedTime();
    
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    
    u8g2->setCursor(85, 8);
    u8g2->print(timeStr.c_str());
    
    int line = 1;
    display.printSensorData("Темп:", temp, "C", line++);
    display.printSensorData("Влажн:", hum, "%", line++);
    display.printSensorData("Лампа:", dsTemp, "C", line++);
    display.printSensorData("Свет:", light, "lx", line++);
    
    u8g2->setCursor(0, 58);
    u8g2->print(relayState.lamp1 ? "[1]" : "[ ]");
    u8g2->setCursor(25, 58);
    u8g2->print(relayState.lamp2 ? "[2]" : "[ ]");
    u8g2->setCursor(50, 58);
    u8g2->print(relayState.lamp3 ? "[3]" : "[ ]");
    u8g2->setCursor(75, 58);
    u8g2->print(relayState.humidifier ? "[H]" : "[ ]");
    
    u8g2->setCursor(105, 58);
    u8g2->print(_controlMode == MODE_AUTO ? "AUTO" : "MAN");
    
    display.update();
}

void Menu::drawModeSelectScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("Выберите режим:");
    
    const char* modes[] = {"AUTO", "MANUAL", "НАСТРОЙКИ"};
    for (int i = 0; i < 3; i++) {
        u8g2->setCursor(10, 28 + i * 14);
        u8g2->print(modes[i]);
        if (_menuPosition == i) u8g2->print(" <");
    }
    display.update();
}

void Menu::drawManualScreen(RelayState relayState) {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    Adafruit_SSD1306* d = display.getDisplay();
    
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    u8g2->setCursor(0, 10);
    u8g2->print("РУЧНОЕ УПРАВЛЕНИЕ");
    d->drawLine(0, 12, 127, 12, SSD1306_WHITE);
    
    int totalItems = 4;
    int visibleItems = 3;
    int startItem = 0;
    
    if (_menuPosition >= startItem + visibleItems) {
        startItem = _menuPosition - visibleItems + 1;
    }
    if (_menuPosition < startItem) {
        startItem = _menuPosition;
    }
    
    const char* names[] = {"Лампа 1", "Лампа 2", "Лампа 3", "Увлажн"};
    bool states[] = {relayState.lamp1, relayState.lamp2, relayState.lamp3, relayState.humidifier};
    
    for (int i = 0; i < visibleItems && (startItem + i) < totalItems; i++) {
        int idx = startItem + i;
        int yPos = 24 + i * 14;
        
        u8g2->setCursor(0, yPos);
        u8g2->print(names[idx]);
        u8g2->setCursor(70, yPos);
        u8g2->print(states[idx] ? "ON " : "OFF");
        
        if (idx == _menuPosition) u8g2->setCursor(100, yPos); u8g2->print("<");
    }
    
    if (startItem > 0) {
        u8g2->setCursor(120, 20);
        u8g2->print("↑");
    }
    if (startItem + visibleItems < totalItems) {
        u8g2->setCursor(120, 60);
        u8g2->print("↓");
    }
    
    u8g2->setCursor(0, 62);
    u8g2->print("Нажмите для ON/OFF");
    
    display.update();
}

void Menu::drawSettingsScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("НАСТРОЙКИ");
    
    if (_menuPosition < 0) _menuPosition = 0;
    if (_menuPosition > 2) _menuPosition = 2;
    
    const char* items[] = {"Лампы", "Увлажнитель", "Статистика"};
    for (int i = 0; i < 3; i++) {
        u8g2->setCursor(10, 28 + i * 14);
        u8g2->print(items[i]);
        if (_menuPosition == i) u8g2->print(" <");
    }
    display.update();
}

void Menu::drawStatsScreen(Statistics stats) {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("СТАТИСТИКА");
    
    char buffer[32];
    
    // Температура
    u8g2->setCursor(0, 24);
    if (stats.tempValid) {
        snprintf(buffer, sizeof(buffer), "T:%.1f-%.1fC", stats.minTemp, stats.maxTemp);
        u8g2->print(buffer);
    } else {
        u8g2->print("T: -- (нет данных)");
    }
    
    // Влажность
    u8g2->setCursor(0, 36);
    if (stats.humValid) {
        snprintf(buffer, sizeof(buffer), "H:%.0f-%.0f%%", stats.minHum, stats.maxHum);
        u8g2->print(buffer);
    } else {
        u8g2->print("H: -- (нет данных)");
    }
    
    // Свет
    u8g2->setCursor(0, 48);
    if (stats.lightValid) {
        snprintf(buffer, sizeof(buffer), "L:%.0f-%.0flx", stats.minLight, stats.maxLight);
        u8g2->print(buffer);
    } else {
        u8g2->print("L: -- (нет данных)");
    }
    
    // Время работы
    unsigned long days = stats.uptime / 86400;
    unsigned long hours = (stats.uptime % 86400) / 3600;
    snprintf(buffer, sizeof(buffer), "Up:%dд %dч", days, hours);
    u8g2->setCursor(0, 60);
    u8g2->print(buffer);
    
    display.update();
}

void Menu::drawHumidifierSettingsScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("УВЛАЖНИТЕЛЬ");
    
    const char* modes[] = {"По порогу", "Циклический", "По расписанию"};
    for (int i = 0; i < 3; i++) {
        u8g2->setCursor(10, 30 + i * 14);
        u8g2->print(modes[i]);
        if (_menuPosition == i) u8g2->print(" <");
    }
    display.update();
}

void Menu::drawHumidifierThresholdScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("ДАТЧИК ВЛАЖНОСТИ");
    
    // Пункт 0: Включено
    u8g2->setCursor(10, 30);
    u8g2->print("Включено:");
    u8g2->setCursor(80, 30);
    u8g2->print(_editHumidifierSensorEnabled ? "[X]" : "[ ]");
    if (_menuPosition == 0 && !_editingActive) {
        u8g2->print(" <");
    }
    
    // Пункт 1: Нижний порог
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Порог вкл: %d %%", _editHumidifierValue);
    u8g2->setCursor(10, 45);
    u8g2->print(buffer);
    if (_menuPosition == 1) {
        if (_editingActive && _editingIndex == 1) {
            u8g2->print(" [ред]");
        } else if (!_editingActive) {
            u8g2->print(" <");
        }
    }
    
    // Пункт 2: Гистерезис
    snprintf(buffer, sizeof(buffer), "Гистерезис: %d %%", _editHumidifierValue2);
    u8g2->setCursor(10, 60);
    u8g2->print(buffer);
    if (_menuPosition == 2) {
        if (_editingActive && _editingIndex == 2) {
            u8g2->print(" [ред]");
        } else if (!_editingActive) {
            u8g2->print(" <");
        }
    }
    
    display.update();
}

void Menu::drawHumidifierCyclicScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("ЦИКЛИЧЕСКИЙ РЕЖИМ");
    
    // Пункт 0: Включено
    u8g2->setCursor(10, 30);
    u8g2->print("Включено:");
    u8g2->setCursor(80, 30);
    u8g2->print(_editHumidifierCyclicEnabled ? "[X]" : "[ ]");
    if (_menuPosition == 0 && !_editingActive) {
        u8g2->print(" <");
    }
    
    // Пункт 1: Время работы
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Работа: %d сек", _editHumidifierValue);
    u8g2->setCursor(10, 45);
    u8g2->print(buffer);
    if (_menuPosition == 1) {
        if (_editingActive && _editingIndex == 1) {
            u8g2->print(" [ред]");
        } else if (!_editingActive) {
            u8g2->print(" <");
        }
    }
    
    // Пункт 2: Время отдыха
    snprintf(buffer, sizeof(buffer), "Отдых: %d сек", _editHumidifierValue2);
    u8g2->setCursor(10, 60);
    u8g2->print(buffer);
    if (_menuPosition == 2) {
        if (_editingActive && _editingIndex == 2) {
            u8g2->print(" [ред]");
        } else if (!_editingActive) {
            u8g2->print(" <");
        }
    }
    
    display.update();
}

void Menu::drawHumidifierScheduleScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("РАСПИСАНИЕ");
    u8g2->setCursor(10, 35);
    u8g2->print("Скоро будет...");
    display.update();
}

void Menu::drawLampSettingsScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("НАСТРОЙКИ ЛАМП");
    
    const char* lamps[] = {"Лампа 1", "Лампа 2", "Лампа 3"};
    for (int i = 0; i < 3; i++) {
        u8g2->setCursor(10, 30 + i * 14);
        u8g2->print(lamps[i]);
        if (_menuPosition == i) u8g2->print(" <");
    }
    display.update();
}

void Menu::drawLampDetailScreen(int lampNumber) {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    
    char title[32];
    snprintf(title, sizeof(title), "ЛАМПА %d", lampNumber);
    u8g2->setCursor(0, 12);
    u8g2->print(title);
    
    // Пункты меню: 0 - Датчик, 1 - Порог, 2 - Гистерезис, 3 - Расписание
    const char* items[] = {"Датчик", "Порог", "Гистерезис", "Расписание"};
    for (int i = 0; i < 4; i++) {
        u8g2->setCursor(10, 28 + i * 10);
        u8g2->print(items[i]);
        if (_menuPosition == i) u8g2->print(" <");
        
        // Отображаем значения из Relay (актуальные) или из временных переменных редактирования
        if (i == 0) {
            // useSensor - всегда берем из Relay
            bool useSensor = false;
            if (_relay) {
                int lampIndex = lampNumber - 1;
                if (lampIndex == 0) useSensor = _relay->getLamp1Settings()->useSensor;
                else if (lampIndex == 1) useSensor = _relay->getLamp2Settings()->useSensor;
                else if (lampIndex == 2) useSensor = _relay->getLamp3Settings()->useSensor;
            }
            u8g2->setCursor(80, 28);
            u8g2->print(useSensor ? "ON" : "OFF");
        } else if (i == 1) {
            // thresholdLow
            int threshold = 0;
            if (_editingActive && _editingIndex == 1) {
                threshold = _editHumidifierValue;
            } else if (_relay) {
                int lampIndex = lampNumber - 1;
                if (lampIndex == 0) threshold = _relay->getLamp1Settings()->thresholdLow;
                else if (lampIndex == 1) threshold = _relay->getLamp2Settings()->thresholdLow;
                else if (lampIndex == 2) threshold = _relay->getLamp3Settings()->thresholdLow;
            }
            char buf[16];
            snprintf(buf, sizeof(buf), "%d lx", threshold);
            u8g2->setCursor(80, 38);
            u8g2->print(buf);
        } else if (i == 2) {
            // hysteresis
            int hysteresis = 0;
            if (_editingActive && _editingIndex == 2) {
                hysteresis = _editHumidifierValue2;
            } else if (_relay) {
                int lampIndex = lampNumber - 1;
                if (lampIndex == 0) hysteresis = _relay->getLamp1Settings()->sensorHysteresis;
                else if (lampIndex == 1) hysteresis = _relay->getLamp2Settings()->sensorHysteresis;
                else if (lampIndex == 2) hysteresis = _relay->getLamp3Settings()->sensorHysteresis;
            }
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", hysteresis);
            u8g2->setCursor(80, 48);
            u8g2->print(buf);
        }
    }
    
    display.update();
}

void Menu::drawLampScheduleScreen(int lampNumber) {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    
    if (!_lampEditor) {
        u8g2->setFont(u8g2_font_6x13_t_cyrillic);
        u8g2->setCursor(0, 12);
        u8g2->print("Ошибка: редактор не инициализирован");
        display.update();
        return;
    }
    
    // Используем номер лампы из редактора (правильный 1-3)
    int actualLampNumber = _lampEditor->getLampNumber();
    
    char title[32];
    snprintf(title, sizeof(title), "ЛАМПА %d РАСПИСАНИЕ", actualLampNumber);
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print(title);
    
    // Пункты меню: 0 - Дни недели, 1 - Время начала, 2 - Время конца
    const char* items[] = {"Дни недели", "Начало", "Конец"};
    for (int i = 0; i < 3; i++) {
        u8g2->setCursor(10, 28 + i * 14);
        u8g2->print(items[i]);
        if (_menuPosition == i) u8g2->print(" <");
    }
    
    // Статус расписания (включено/выключено)
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    u8g2->setCursor(0, 35);
    u8g2->print(_lampEditor->getUseSchedule() ? "Статус: ВКЛ" : "Статус: ВЫКЛ");
    
    // Отображаем времена с выделением активного поля
    char timeStr[32];
    int activeField = _lampEditor->getActiveField();
    int editMode = _lampEditor->getEditMode();
    
    // Время начала
    if (editMode == 1 && activeField >= 0) {
        if (activeField == 0) {
            snprintf(timeStr, sizeof(timeStr), "Нач: *%02d:%02d", 
                     _lampEditor->getEditHour(), _lampEditor->getScheduleStartMin());
        } else {
            snprintf(timeStr, sizeof(timeStr), "Нач: %02d:*%02d", 
                     _lampEditor->getScheduleStartHour(), _lampEditor->getEditMinute());
        }
    } else {
        snprintf(timeStr, sizeof(timeStr), "Нач: %02d:%02d", 
                 _lampEditor->getScheduleStartHour(), _lampEditor->getScheduleStartMin());
    }
    u8g2->setCursor(0, 48);
    u8g2->print(timeStr);
    
    // Время конца
    if (editMode == 2 && activeField >= 0) {
        if (activeField == 0) {
            snprintf(timeStr, sizeof(timeStr), "Кон:  *%02d:%02d", 
                     _lampEditor->getEditHour(), _lampEditor->getScheduleEndMin());
        } else {
            snprintf(timeStr, sizeof(timeStr), "Кон:  %02d:*%02d", 
                     _lampEditor->getScheduleEndHour(), _lampEditor->getEditMinute());
        }
    } else {
        snprintf(timeStr, sizeof(timeStr), "Кон:  %02d:%02d", 
                 _lampEditor->getScheduleEndHour(), _lampEditor->getScheduleEndMin());
    }
    u8g2->setCursor(0, 58);
    u8g2->print(timeStr);
    
    display.update();
}

void Menu::drawLampDaysScreen() {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("ДНИ НЕДЕЛИ");
    
    const char* days[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
    
    if (!_lampEditor) {
        u8g2->setFont(u8g2_font_5x8_t_cyrillic);
        u8g2->setCursor(0, 35);
        u8g2->print("Ошибка: редактор не инициализирован");
        display.update();
        return;
    }
    
    // Показываем дни в одну строку с индикацией выбранного дня
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    for (int i = 0; i < 7; i++) {
        u8g2->setCursor(10 + i * 17, 35);
        if (_menuPosition == i) {
            u8g2->print(">");  // индикатор выбранного дня
        } else {
            u8g2->print(" ");
        }
        u8g2->print(days[i]);
        
        if (_lampEditor->getScheduleDay(i)) {
            u8g2->setCursor(10 + i * 17, 50);
            u8g2->print("[X]");
        }
    }
    
    display.update();
}

void Menu::drawConfirmScreen(const char* message) {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 30);
    u8g2->print(message);
    u8g2->setCursor(0, 50);
    u8g2->print("Нажмите для подтверждения");
    display.update();
}