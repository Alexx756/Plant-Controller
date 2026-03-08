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
    
    // Флаг принудительного обновления AUTO
    _forceAutoUpdate = false;
    
    _controlMode = MODE_AUTO;
    
    _thresholds.lightThreshold = LIGHT_THRESHOLD;
    _thresholds.humidityThreshold = HUMIDITY_THRESHOLD;
    _thresholds.tempThreshold = TEMP_THRESHOLD;
    
    _editHumidifierMode = 0;
    _editHumidifierValue = 0;
    
    _manualToggleRequested = false;
    _manualToggleIndex = 0;
    
    menuInstance = this;
}

void Menu::testEncoder() {
    Serial.println("=== ТЕСТ ЭНКОДЕРА ===");
    Serial.printf("Пин CLK: %d\n", ENCODER_CLK);
    Serial.printf("Пин DT: %d\n", ENCODER_DT);
    Serial.printf("Пин SW: %d\n", ENCODER_SW);
    
    // Проверка чтения пинов напрямую
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

void Menu::begin() {
   Serial.println("🔄 Инициализация энкодера...");
    
    // Сначала настроим пины как INPUT_PULLUP
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    // Создаем объект энкодера
    _encoder = new AiEsp32RotaryEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, -1, 4);
    
    // Настраиваем параметры
    _encoder->begin();
    _encoder->setup(readEncoderISR);
    _encoder->setAcceleration(0);
    _encoder->disableAcceleration();
    
    Serial.println("✅ Энкодер инициализирован");
    
    // Запускаем тест
    testEncoder();
}

// ISR функция (обработчик прерывания)
void IRAM_ATTR Menu::readEncoderISR() {
    if (menuInstance && menuInstance->_encoder) {
        menuInstance->_encoder->readEncoder_ISR();
    }
}

void Menu::update(float temp, float hum, float dsTemp, float light, RelayState relayState, Statistics stats) {
    
    if (!_encoder) return;
    
    // Чтение энкодера
    static long lastValue = 0;
    long currentValue = _encoder->readEncoder();
    
    if (currentValue != lastValue) {
        int delta = (currentValue > lastValue) ? 1 : -1;
        
        // Логируем вращение
        if (delta != 0) {
            logger.logEncoder(delta, _menuPosition);
        }
        
        // Вызываем обработчик вращения
        handleEncoderRotation(delta);
        
        lastValue = currentValue;
    }
    
    // ===== ОБРАБОТКА КНОПКИ =====
    static unsigned long pressStartTime = 0;
    static bool longPressHandled = false;
    bool buttonDown = _encoder->isEncoderButtonDown();
    
    if (buttonDown) {
        // Кнопка нажата
        if (pressStartTime == 0) {
            // Только что нажали
            pressStartTime = millis();
            longPressHandled = false;
            logger.logEncoder(0, 999); // отладка: кнопка нажата
        } else if (!longPressHandled && (millis() - pressStartTime > 1000)) {
            // Долгое нажатие (больше 1 секунды)
            logger.logEncoder(0, 998); // отладка: долгое нажатие
            handleLongPress();
            longPressHandled = true;
        }
    } else {
        // Кнопка отпущена
        if (pressStartTime != 0 && !longPressHandled) {
            // Короткое нажатие (отпустили до 1 секунды)
            logger.logEncoder(0, 997); // отладка: короткое нажатие
            handleShortPress();
        }
        // Сбрасываем все флаги
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
            if (_menuPosition < 0) _menuPosition = 3;  // 4 пункта: 0,1,2,3
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
            _menuPosition += delta;
            if (_menuPosition < 0) _menuPosition = 2;
            if (_menuPosition > 2) _menuPosition = 0;
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
                    _forceAutoUpdate = true;  // флаг принудительного обновления
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
                case 0: // Пороги
                    _currentScreen = SCREEN_THRESHOLDS;
                    _menuPosition = 0;
                    break;
                case 1: // Лампы
                    _currentScreen = SCREEN_LAMP_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 2: // Увлажнитель
                    _currentScreen = SCREEN_HUMIDIFIER_SETTINGS;
                    _menuPosition = 0;
                    break;
                case 3: // Статистика
                    _currentScreen = SCREEN_STATS;
                    _menuPosition = 0;
                    break;
            }
            break;
            
        case SCREEN_THRESHOLDS:
            Serial.printf("Выбран порог %d для редактирования\n", _menuPosition);
            // TODO: добавить редактирование порогов
            break;
            
        case SCREEN_STATS:
            _currentScreen = SCREEN_SETTINGS;
            break;
            
        case SCREEN_HUMIDIFIER_SETTINGS:
            // Выбор режима увлажнителя
            switch (_menuPosition) {
                case 0: // По порогу
                    _currentScreen = SCREEN_HUMIDIFIER_THRESHOLD;
                    _editHumidifierValue = HUMIDITY_THRESHOLD;
                    break;
                case 1: // Циклический
                    _currentScreen = SCREEN_HUMIDIFIER_CYCLIC;
                    _editHumidifierValue = HUMIDIFIER_WORK_TIME;
                    break;
                case 2: // По расписанию
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
            // Сохраняем порог
            if (_scheduler) {
                _scheduler->setHumidifierThreshold(_editHumidifierValue);
            }
            _currentScreen = SCREEN_SETTINGS;
            break;
            
        case SCREEN_HUMIDIFIER_CYCLIC:
            // Сохраняем циклические параметры
            if (_scheduler) {
                _scheduler->setHumidifierCyclic(_editHumidifierValue, _editHumidifierValue * 10);
            }
            _currentScreen = SCREEN_SETTINGS;
            break;
            
        case SCREEN_LAMP_SETTINGS:
            // Выбор лампы для настройки
            switch (_menuPosition) {
                case 0:
                    _currentScreen = SCREEN_LAMP1_SETTINGS;
                    break;
                case 1:
                    _currentScreen = SCREEN_LAMP2_SETTINGS;
                    break;
                case 2:
                    _currentScreen = SCREEN_LAMP3_SETTINGS;
                    break;
            }
            _menuPosition = 0;
            break;
            
        case SCREEN_CONFIRM:
            _currentScreen = _prevScreen;
            break;
            
        default:
            break;
    }
}

void Menu::handleLongPress() {
    Serial.println("Долгое нажатие - возврат на главный");
    _currentScreen = SCREEN_MAIN;
    _menuPosition = 0;
}

// ===== ФУНКЦИИ ОТРИСОВКИ =====

void Menu::drawMainScreen(float temp, float hum, float dsTemp, float light, RelayState relayState) {
    display.clear();
    
    extern TimeManager timeManager;
    String timeStr = timeManager.getFormattedTime();
    
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    
    // Время в правом верхнем углу
    u8g2->setCursor(85, 8);
    u8g2->print(timeStr.c_str());
    
    // Датчики
    int line = 1;
    display.printSensorData("Темп:", temp, "C", line++);
    display.printSensorData("Влажн:", hum, "%", line++);
    display.printSensorData("Лампа:", dsTemp, "C", line++);
    display.printSensorData("Свет:", light, "lx", line++);
    
    // Строка состояния реле
    u8g2->setCursor(0, 58);
    u8g2->print(relayState.lamp1 ? "[1]" : "[ ]");
    u8g2->setCursor(25, 58);
    u8g2->print(relayState.lamp2 ? "[2]" : "[ ]");
    u8g2->setCursor(50, 58);
    u8g2->print(relayState.lamp3 ? "[3]" : "[ ]");
    u8g2->setCursor(75, 58);
    u8g2->print(relayState.humidifier ? "[H]" : "[ ]");
    
    // Режим AUTO/MAN
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
        if (_menuPosition == i) {
            u8g2->print(" <");
        }
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
        
        if (idx == _menuPosition) {
            u8g2->setCursor(100, yPos);
            u8g2->print("<");
        }
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

        // Защита: если _menuPosition вдруг вышел за границы, возвращаем
    if (_menuPosition < 0) _menuPosition = 0;
    if (_menuPosition > 3) _menuPosition = 3;
    
    const char* items[] = {"Пороги", "Лампы", "Увлажнитель", "Статистика"};
    for (int i = 0; i < 4; i++) {
        u8g2->setCursor(10, 28 + i * 14);
        u8g2->print(items[i]);
        if (_menuPosition == i) {
            u8g2->print(" <");
        }
    }
    
    display.update();
}

void Menu::drawThresholdsScreen() {
    display.clear();
    
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("ПОРОГИ");
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Свет: %d lx", _thresholds.lightThreshold);
    u8g2->setCursor(10, 28);
    u8g2->print(buffer);
    if (_menuPosition == 0) u8g2->print(" <");
    
    snprintf(buffer, sizeof(buffer), "Влажн: %d%%", _thresholds.humidityThreshold);
    u8g2->setCursor(10, 42);
    u8g2->print(buffer);
    if (_menuPosition == 1) u8g2->print(" <");
    
    snprintf(buffer, sizeof(buffer), "Темп: %.1f°C", _thresholds.tempThreshold);
    u8g2->setCursor(10, 56);
    u8g2->print(buffer);
    if (_menuPosition == 2) u8g2->print(" <");
    
    display.update();
}

void Menu::drawStatsScreen(Statistics stats) {
    display.clear();
    
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("СТАТИСТИКА");
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "T:%.1f-%.1fC", stats.minTemp, stats.maxTemp);
    u8g2->setCursor(0, 24);
    u8g2->print(buffer);
    
    snprintf(buffer, sizeof(buffer), "H:%.0f-%.0f%%", stats.minHum, stats.maxHum);
    u8g2->setCursor(0, 36);
    u8g2->print(buffer);
    
    snprintf(buffer, sizeof(buffer), "L:%.0f-%.0flx", stats.minLight, stats.maxLight);
    u8g2->setCursor(0, 48);
    u8g2->print(buffer);
    
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
        if (_menuPosition == i) {
            u8g2->print(" <");
        }
    }
    
    display.update();
}

void Menu::drawHumidifierThresholdScreen() {
    display.clear();
    
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("ПОРОГ ВЛАЖНОСТИ");
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d %%", _editHumidifierValue);
    u8g2->setCursor(10, 40);
    u8g2->print(buffer);
    
    u8g2->setCursor(0, 58);
    u8g2->print("Вращайте для изменения");
    
    display.update();
}

void Menu::drawHumidifierCyclicScreen() {
    display.clear();
    
    U8G2_FOR_ADAFRUIT_GFX* u8g2 = display.getU8g2();
    u8g2->setFont(u8g2_font_6x13_t_cyrillic);
    u8g2->setCursor(0, 12);
    u8g2->print("ЦИКЛИЧЕСКИЙ РЕЖИМ");
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Работа: %d сек", _editHumidifierValue);
    u8g2->setCursor(10, 35);
    u8g2->print(buffer);
    
    snprintf(buffer, sizeof(buffer), "Отдых: %d сек", _editHumidifierValue * 10);
    u8g2->setCursor(10, 50);
    u8g2->print(buffer);
    
    u8g2->setCursor(0, 62);
    u8g2->setFont(u8g2_font_5x8_t_cyrillic);
    u8g2->print("Вращайте для работы");
    
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
        if (_menuPosition == i) {
            u8g2->print(" <");
        }
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
    
    // Здесь будут настройки конкретной лампы
    u8g2->setCursor(0, 35);
    u8g2->print("Режим: AUTO");
    u8g2->setCursor(0, 50);
    u8g2->print("Порог: 200 lx");
    
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
