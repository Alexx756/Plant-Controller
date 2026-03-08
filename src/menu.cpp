#include "menu.h"
#include "display.h"
#include "logger.h"
#include "config.h"
#include <Arduino.h>

extern Display display;
Menu* menuInstance = nullptr;

// ===================== КОНСТРУКТОР =====================
Menu::Menu() {
    _currentScreen = SCREEN_MAIN;
    _prevScreen = SCREEN_MAIN;
    _menuPosition = 0;

    _encoder = nullptr;
    _encoderPressed = false;
    _pressStartTime = 0;
    _longPressTriggered = false;

    _scheduler = nullptr;
    _forceAutoUpdate = false;
    _controlMode = MODE_AUTO;

    _thresholds.lightThreshold = LIGHT_THRESHOLD;
    _thresholds.humidityThreshold = HUMIDITY_THRESHOLD;
    _thresholds.tempThreshold = TEMP_THRESHOLD;

    _editHumidifierMode = 0;
    _editHumidifierValue = 0;

    _manualToggleRequested = false;
    _manualToggleIndex = 0;

    // Для ламп
    _relays = nullptr;
    _editing = false;
    _editingItem = 0;
    _editLampIndex = 0;

    menuInstance = this;
}

// ===================== ТЕСТ ЭНКОДЕРА =====================
void Menu::testEncoder() {
    Serial.println("=== ТЕСТ ЭНКОДЕРА ===");
    Serial.printf("Пин CLK: %d\n", ENCODER_CLK);
    Serial.printf("Пин DT: %d\n", ENCODER_DT);
    Serial.printf("Пин SW: %d\n", ENCODER_SW);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    for(int i = 0; i < 20; i++) {
        int clk = digitalRead(ENCODER_CLK);
        int dt = digitalRead(ENCODER_DT);
        int sw = digitalRead(ENCODER_SW);
        Serial.printf("CLK=%d DT=%d SW=%d\n", clk, dt, sw);
        delay(500);
    }
}

// ===================== ИНИЦИАЛИЗАЦИЯ =====================
void Menu::begin() {
    Serial.println("🔄 Инициализация энкодера...");
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    _encoder = new AiEsp32RotaryEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, -1, 4);
    _encoder->begin();
    _encoder->setup(readEncoderISR);
    _encoder->setAcceleration(0);
    _encoder->disableAcceleration();
    Serial.println("✅ Энкодер инициализирован");
    testEncoder();
}

// ===================== ISR =====================
void IRAM_ATTR Menu::readEncoderISR() {
    if (menuInstance && menuInstance->_encoder) {
        menuInstance->_encoder->readEncoder_ISR();
    }
}

// ===================== ОСНОВНОЙ ЦИКЛ ОБНОВЛЕНИЯ =====================
void Menu::update(float temp, float hum, float dsTemp, float light, RelayState relayState, Statistics stats) {
    if (!_encoder) return;

    // Чтение энкодера
    static long lastValue = 0;
    long currentValue = _encoder->readEncoder();
    if (currentValue != lastValue) {
        int delta = (currentValue > lastValue) ? 1 : -1;
        handleEncoderRotation(delta);
        lastValue = currentValue;
    }

    // Обработка кнопки
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

    // Отрисовка экрана
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
        case SCREEN_THRESHOLDS:
            drawThresholdsScreen();
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
        case SCREEN_LAMP_SETTINGS:
            drawLampSettingsScreen();
            break;
        case SCREEN_LAMP1_SETTINGS:
        case SCREEN_LAMP2_SETTINGS:
        case SCREEN_LAMP3_SETTINGS:
            drawLampDetailScreen(_currentScreen - SCREEN_LAMP1_SETTINGS + 1);
            break;
        default:
            break;
    }
}

// ===================== ОБРАБОТКА ВРАЩЕНИЯ =====================
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
        case SCREEN_THRESHOLDS:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
            break;
        case SCREEN_HUMIDIFIER_SETTINGS:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
            break;
        case SCREEN_HUMIDIFIER_THRESHOLD:
            _editHumidifierValue += delta * 5;
            if (_editHumidifierValue < 20) _editHumidifierValue = 20;
            if (_editHumidifierValue > 90) _editHumidifierValue = 90;
            break;
        case SCREEN_HUMIDIFIER_CYCLIC:
            _editHumidifierValue += delta * 10;
            if (_editHumidifierValue < 10) _editHumidifierValue = 10;
            if (_editHumidifierValue > 600) _editHumidifierValue = 600;
            break;
        case SCREEN_LAMP_SETTINGS:
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
            break;
        case SCREEN_LAMP1_SETTINGS:
        case SCREEN_LAMP2_SETTINGS:
        case SCREEN_LAMP3_SETTINGS:
            handleLampEncoder(delta);
            break;
        default:
            break;
    }
}

// ===================== ОБРАБОТКА КОРОТКОГО НАЖАТИЯ =====================
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
                case 0:
                    _controlMode = MODE_AUTO;
                    _currentScreen = SCREEN_MAIN;
                    _forceAutoUpdate = true;
                    logger.logMenu("→ AUTO режим", _currentScreen);
                    break;
                case 1:
                    _controlMode = MODE_MANUAL;
                    _currentScreen = SCREEN_MANUAL;
                    _menuPosition = 0;
                    logger.logMenu("→ MANUAL режим", _currentScreen);
                    break;
                case 2:
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
                case 0:
                    _currentScreen = SCREEN_THRESHOLDS;
                    _menuPosition = 0;
                    break;
                case 1:
                    _currentScreen = SCREEN_LAMP_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 2:
                    _currentScreen = SCREEN_HUMIDIFIER_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 3:
                    _currentScreen = SCREEN_STATS;
                    _menuPosition = 0;
                    break;
            }
            break;
        case SCREEN_THRESHOLDS:
            Serial.printf("Выбран порог %d для редактирования\n", _menuPosition);
            // TODO: редактирование порогов
            break;
        case SCREEN_STATS:
            _currentScreen = SCREEN_SETTINGS;
            break;
        case SCREEN_HUMIDIFIER_SETTINGS:
            switch (_menuPosition) {
                case 0:
                    _currentScreen = SCREEN_HUMIDIFIER_THRESHOLD;
                    _editHumidifierValue = HUMIDITY_THRESHOLD;
                    break;
                case 1:
                    _currentScreen = SCREEN_HUMIDIFIER_CYCLIC;
                    _editHumidifierValue = HUMIDIFIER_WORK_TIME;
                    break;
                case 2:
                    if (_scheduler) {
                        _scheduler->setHumidifierSchedule(8, 0, 22, 0, nullptr, 0);
                        logger.log("УВЛАЖНИТЕЛЬ", "Режим: по расписанию (8:00-22:00)");
                    }
                    _currentScreen = SCREEN_SETTINGS;
                    break;
            }
            _menuPosition = 0;
            break;
        case SCREEN_HUMIDIFIER_THRESHOLD:
            if (_scheduler) {
                _scheduler->setHumidifierThreshold(_editHumidifierValue);
            }
            _currentScreen = SCREEN_SETTINGS;
            break;
        case SCREEN_HUMIDIFIER_CYCLIC:
            if (_scheduler) {
                _scheduler->setHumidifierCyclic(_editHumidifierValue, _editHumidifierValue * 10);
            }
            _currentScreen = SCREEN_SETTINGS;
            break;
        case SCREEN_LAMP_SETTINGS:
            switch (_menuPosition) {
                case 0:
                    _currentScreen = SCREEN_LAMP1_SETTINGS;
                    _editLampIndex = 1;
                    if (_relays) _editLamp = *_relays->getLamp1Settings();
                    break;
                case 1:
                    _currentScreen = SCREEN_LAMP2_SETTINGS;
                    _editLampIndex = 2;
                    if (_relays) _editLamp = *_relays->getLamp2Settings();
                    break;
                case 2:
                    _currentScreen = SCREEN_LAMP3_SETTINGS;
                    _editLampIndex = 3;
                    if (_relays) _editLamp = *_relays->getLamp3Settings();
                    break;
            }
            _menuPosition = 0;
            _editing = false;
            break;
        case SCREEN_LAMP1_SETTINGS:
        case SCREEN_LAMP2_SETTINGS:
        case SCREEN_LAMP3_SETTINGS:
            handleLampPress();
            break;
        case SCREEN_CONFIRM:
            _currentScreen = _prevScreen;
            break;
        default:
            break;
    }
}

// ===================== ОБРАБОТКА ДОЛГОГО НАЖАТИЯ =====================
void Menu::handleLongPress() {
    Serial.println("Долгое нажатие - возврат на главный");
    // Сохраняем настройки лампы, если мы на её экране
    if (_currentScreen == SCREEN_LAMP1_SETTINGS ||
        _currentScreen == SCREEN_LAMP2_SETTINGS ||
        _currentScreen == SCREEN_LAMP3_SETTINGS) {
        saveCurrentLampSettings();
    }
    _currentScreen = SCREEN_MAIN;
    _menuPosition = 0;
}

// ===================== ОТРИСОВКА ЭКРАНОВ =====================
// (оставляем как в предыдущих версиях – они у вас уже должны быть)
// Здесь привожу только новые функции для ламп, остальные вы уже имеете.

void Menu::drawLampDetailScreen(int lampNumber) {
    display.clear();
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);

    char title[32];
    snprintf(title, sizeof(title), "ЛАМПА %d", lampNumber);
    u8g2->setCursor(0, 12);
    u8g2->print(title);

    int y = 28;
    int currentItem = 0;

    bool useSensor = (_editLamp.mode == CH_MODE_SENSOR || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
    bool useSchedule = (_editLamp.mode == CH_MODE_SCHEDULE || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);

    // Чекбокс датчика
    u8g2->setCursor(10, y);
    u8g2->print(useSensor ? "[x] " : "[ ] ");
    u8g2->print("По датчику");
    if (!_editing && _menuPosition == currentItem) u8g2->print(" <");
    y += 14;
    currentItem++;

    if (useSensor) {
        char buf[32];
        snprintf(buf, sizeof(buf), "  Нижний: %d lx", _editLamp.thresholdLow);
        u8g2->setCursor(10, y); u8g2->print(buf);
        if (!_editing && _menuPosition == currentItem) u8g2->print(" <");
        y += 14; currentItem++;

        snprintf(buf, sizeof(buf), "  Верхний: %d lx", _editLamp.thresholdHigh);
        u8g2->setCursor(10, y); u8g2->print(buf);
        if (!_editing && _menuPosition == currentItem) u8g2->print(" <");
        y += 14; currentItem++;
    }

    // Чекбокс расписания
    u8g2->setCursor(10, y);
    u8g2->print(useSchedule ? "[x] " : "[ ] ");
    u8g2->print("По расписанию");
    if (!_editing && _menuPosition == currentItem) u8g2->print(" <");
    y += 14; currentItem++;

    if (useSchedule) {
        char buf[32];
        snprintf(buf, sizeof(buf), "  Вкл: %02d:%02d", _editLamp.scheduleStartHour, _editLamp.scheduleStartMin);
        u8g2->setCursor(10, y); u8g2->print(buf);
        if (!_editing && _menuPosition == currentItem) u8g2->print(" <");
        y += 14; currentItem++;

        snprintf(buf, sizeof(buf), "  Выкл: %02d:%02d", _editLamp.scheduleEndHour, _editLamp.scheduleEndMin);
        u8g2->setCursor(10, y); u8g2->print(buf);
        if (!_editing && _menuPosition == currentItem) u8g2->print(" <");
        // y не нужен далее
    }

    if (_editing) {
        u8g2->setCursor(0, 62);
        u8g2->setFont(u8g2_font_5x8_t_cyrillic);
        u8g2->print("Редактирование...");
    }

    display.update();
}

// ===================== ОБРАБОТЧИКИ ДЛЯ ЛАМП =====================
void Menu::handleLampPress() {
    if (_editing) {
        _editing = false;
        return;
    }
    int pos = _menuPosition;
    int idx = 0;

    // Чекбокс датчика
    if (pos == idx) {
        bool useSensor = (_editLamp.mode == CH_MODE_SENSOR || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
        bool useSchedule = (_editLamp.mode == CH_MODE_SCHEDULE || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
        if (useSensor) {
            // Выключаем датчик
            _editLamp.mode = useSchedule ? CH_MODE_SCHEDULE : CH_MODE_OFF;
        } else {
            // Включаем датчик
            _editLamp.mode = useSchedule ? CH_MODE_SENSOR_SCHEDULE : CH_MODE_SENSOR;
        }
        _menuPosition = 0;
        return;
    }
    idx++;

    bool useSensor = (_editLamp.mode == CH_MODE_SENSOR || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
    if (useSensor) {
        if (pos == idx) {
            _editing = true; _editingItem = 1; return; // нижний
        }
        idx++;
        if (pos == idx) {
            _editing = true; _editingItem = 2; return; // верхний
        }
        idx++;
    }

    // Чекбокс расписания
    if (pos == idx) {
        bool useSensor = (_editLamp.mode == CH_MODE_SENSOR || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
        bool useSchedule = (_editLamp.mode == CH_MODE_SCHEDULE || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
        if (useSchedule) {
            _editLamp.mode = useSensor ? CH_MODE_SENSOR : CH_MODE_OFF;
        } else {
            _editLamp.mode = useSensor ? CH_MODE_SENSOR_SCHEDULE : CH_MODE_SCHEDULE;
        }
        _menuPosition = 0;
        return;
    }
    idx++;

    bool useSchedule = (_editLamp.mode == CH_MODE_SCHEDULE || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE);
    if (useSchedule) {
        if (pos == idx) { _editing = true; _editingItem = 3; return; } // час вкл
        idx++;
        if (pos == idx) { _editing = true; _editingItem = 4; return; } // мин вкл
        idx++;
        if (pos == idx) { _editing = true; _editingItem = 5; return; } // час выкл
        idx++;
        if (pos == idx) { _editing = true; _editingItem = 6; return; } // мин выкл
    }
}

void Menu::handleLampEncoder(int delta) {
    if (!_editing) {
        // Навигация по пунктам
        int maxItem = 0;
        maxItem++; // чекбокс датчика
        if (_editLamp.mode == CH_MODE_SENSOR || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE) maxItem += 2;
        maxItem++; // чекбокс расписания
        if (_editLamp.mode == CH_MODE_SCHEDULE || _editLamp.mode == CH_MODE_SENSOR_SCHEDULE) maxItem += 2;

        _menuPosition += delta;
        if (_menuPosition < 0) _menuPosition = maxItem - 1;
        if (_menuPosition >= maxItem) _menuPosition = 0;
        return;
    }

    // Редактирование значения
    switch (_editingItem) {
        case 1:
            _editLamp.thresholdLow += delta * 5;
            if (_editLamp.thresholdLow < 0) _editLamp.thresholdLow = 0;
            if (_editLamp.thresholdLow > 1000) _editLamp.thresholdLow = 1000;
            break;
        case 2:
            _editLamp.thresholdHigh += delta * 5;
            if (_editLamp.thresholdHigh < 0) _editLamp.thresholdHigh = 0;
            if (_editLamp.thresholdHigh > 1000) _editLamp.thresholdHigh = 1000;
            break;
        case 3:
            _editLamp.scheduleStartHour += delta;
            if (_editLamp.scheduleStartHour < 0) _editLamp.scheduleStartHour = 23;
            if (_editLamp.scheduleStartHour > 23) _editLamp.scheduleStartHour = 0;
            break;
        case 4:
            _editLamp.scheduleStartMin += delta * 5;
            if (_editLamp.scheduleStartMin < 0) _editLamp.scheduleStartMin = 55;
            if (_editLamp.scheduleStartMin > 59) _editLamp.scheduleStartMin = 0;
            break;
        case 5:
            _editLamp.scheduleEndHour += delta;
            if (_editLamp.scheduleEndHour < 0) _editLamp.scheduleEndHour = 23;
            if (_editLamp.scheduleEndHour > 23) _editLamp.scheduleEndHour = 0;
            break;
        case 6:
            _editLamp.scheduleEndMin += delta * 5;
            if (_editLamp.scheduleEndMin < 0) _editLamp.scheduleEndMin = 55;
            if (_editLamp.scheduleEndMin > 59) _editLamp.scheduleEndMin = 0;
            break;
    }
}

void Menu::saveCurrentLampSettings() {
    if (!_relays) return;
    ChannelSettings* target = nullptr;
    switch (_editLampIndex) {
        case 1: target = _relays->getLamp1Settings(); break;
        case 2: target = _relays->getLamp2Settings(); break;
        case 3: target = _relays->getLamp3Settings(); break;
    }
    if (target) *target = _editLamp;
}