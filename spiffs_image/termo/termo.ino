#include <LiquidCrystal_I2C.h>



// ============================================================
// RadiatorFanController.ino — Arduino охлаждения радиатора
// Пины: FAN1=9, FAN2=10, NTC=A0, TEST_MODE=12
// SoftwareSerial: RX=8, TX=11, 115200 бод
// EEPROM: калибровки + tempOffset
// CRC8 на приём и передачу
// Тестовый режим с неблокирующим вводом и отображением ШИМ
// Таймер 1 настроен на 31.25 кГц для устранения писка
// LCD 1602 I2C (0x27) с обновлением только изменённых данных
// Heartbeat каждые 5 с (ШИМ1, ШИМ2, температура×10)
// ============================================================

#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// --------------------------- ПИНЫ ---------------------------
#define FAN1_PWM            9   // ШИМ вентилятора 1 (горячая сторона) - таймер 1
#define FAN2_PWM            10  // ШИМ вентилятора 2 (холодная сторона) - таймер 1
#define TEMP_PIN            A0
#define TEST_MODE_PIN       12

// ------------------- ПРОГРАММНЫЙ UART ------------------------
#define SOFT_RX_PIN         8
#define SOFT_TX_PIN         11
SoftwareSerial softSerial(SOFT_RX_PIN, SOFT_TX_PIN);

// ------------------- LCD 1602 I2C ---------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // адрес 0x27 (стандарт) или 0x3F

// ----------------------- ПАРАМЕТРЫ NTC -----------------------
#define NTC_SERIES_RESISTOR  10000.0f
#define NTC_REF_RESISTOR     10000.0f
#define NTC_REF_TEMP         298.15f
#define NTC_BETA             3950.0f

// --------------------- ТЕМПЕРАТУРНЫЕ ПОРОГИ ------------------
#define TEMP_NORMAL_TARGET   100.0f
#define TEMP_NORMAL_START    98.0f
#define TEMP_NORMAL_STOP     96.0f
#define TEMP_HIGHWAY_START   100.0f
#define TEMP_HIGHWAY_HOT     103.0f
#define TEMP_CITY_START      98.0f
#define TEMP_CITY_HOT        108.0f
#define TEMP_OVERHEAT        115.0f
#define TEMP_COLD_LIMIT      85.0f

// ---------------------- ПАРАМЕТРЫ ДОГОНЯЛОК ------------------
#define STEP_PWM             1       // шаг изменения ШИМ
#define DIFF_THRESHOLD_LOW   5       // порог разницы до 50% ШИМ
#define DIFF_THRESHOLD_HIGH  10      // порог разницы выше 50% ШИМ
#define PWM_50_PERCENT       127
#define PWM_75_PERCENT       191
#define PWM_70_PERCENT       178
#define PWM_30_PERCENT       76

// ---------------------- ТАЙМЕРЫ ------------------------------
#define PROCESS_INTERVAL     50      // период вызова processFans, мс
#define HEARTBEAT_INTERVAL   5000    // период heartbeat, мс
#define LCD_INTERVAL         250     // период обновления LCD, мс

// ---------------------- ТАЙМАУТ ПРОБКИ -----------------------
#define TRAFFIC_TIMEOUT_MS   300000UL
#define TRAFFIC_EXIT_MS      300000UL

// ---------------------- КОНСТАНТЫ UART -----------------------
#define UART_BAUD            115200
#define FRAME_MAGIC          0xAA
#define FRAME_TOTAL_SIZE     10
#define ADDR_ARDUINO         0x01
#define ADDR_ESP32_GW        0x02
#define ADDR_ESP32_P4        0x03

// -------------------- КОМАНДЫ ПРОТОКОЛА ----------------------
#define MSG_FAN_SET_MODE     0xC0
#define MSG_FAN_SET_PWM1     0xC1
#define MSG_FAN_SET_PWM2     0xC2
#define MSG_FAN_SET_AUTO     0xC3
#define MSG_FAN_TELEMETRY    0xC8
#define MSG_FAN_CALIB_START_POINT 0xCC
#define MSG_FAN_CALIB_STOP_POINT  0xCD
#define MSG_FAN_CALIB_NOISE_LOW   0xCE
#define MSG_FAN_CALIB_NOISE_HIGH  0xCF
#define MSG_FAN_CALIB_SAVE        0xD0
#define MSG_TEMP_OFFSET_SET  0xE0
#define MSG_TEMP_OFFSET_GET  0xE1
#define MSG_HEARTBEAT_RSP    0x61   // heartbeat ответ (с данными)

// ----------------------- РЕЖИМЫ ------------------------------
enum FanMode { MODE_NORMAL = 1, MODE_HIGHWAY = 2, MODE_CITY = 3 };

// ============== СТРУКТУРЫ ДЛЯ EEPROM =========================
struct Calibration {
    uint8_t startPoint;
    uint8_t stopPoint;
    uint8_t noiseLow;
    uint8_t noiseHigh;
};

struct EEPROMData {
    uint8_t magic;
    Calibration calib1;
    Calibration calib2;
    float tempOffset;
};

// ============== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =======================
FanMode currentMode = MODE_NORMAL;
bool manualMode = false;
uint8_t manualPWM1 = 0;
uint8_t manualPWM2 = 0;

uint8_t fan1PWM = 0;
uint8_t fan2PWM = 0;

Calibration calib1 = {65, 30, 170, 200};
Calibration calib2 = {65, 30, 170, 200};
float tempOffset = 0.0f;

uint32_t trafficTimer = 0;
bool trafficTimerActive = false;
uint32_t normalTimer = 0;
bool normalTimerActive = false;

bool startupPhase = false;
uint32_t startupTimer = 0;
bool startupDropped = false;

uint8_t rxBuffer[FRAME_TOTAL_SIZE];
uint8_t rxIndex = 0;
bool frameStarted = false;

bool testMode = false;
float testTempValue = 90.0;

uint32_t lastRxTime = 0;          // для статуса связи
bool modeFromScreen = false;      // true, если режим задан с экрана

String lcdLine0 = "";
String lcdLine1 = "";

// ==================== CRC8 ====================================
uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x01) crc = (crc >> 1) ^ 0x8C;
            else crc >>= 1;
        }
    }
    return crc;
}

// ==================== EEPROM ФУНКЦИИ =========================
void loadCalibration() {
    EEPROMData data;
    EEPROM.get(0, data);
    if (data.magic == 0xA5) {
        calib1 = data.calib1;
        calib2 = data.calib2;
        tempOffset = data.tempOffset;
    }
}

void saveCalibration() {
    EEPROMData data;
    data.magic = 0xA5;
    data.calib1 = calib1;
    data.calib2 = calib2;
    data.tempOffset = tempOffset;
    EEPROM.put(0, data);
}

// ==================== ЧТЕНИЕ ТЕМПЕРАТУРЫ =====================
float readCoolantTemp() {
    if (testMode) return testTempValue;

    int raw = analogRead(TEMP_PIN);
    float vout = raw * 5.0f / 1023.0f;
    float r = NTC_SERIES_RESISTOR * ((5.0f / vout) - 1.0f);
    float tempK = 1.0f / ((1.0f/NTC_REF_TEMP) + (1.0f/NTC_BETA)*log(r/NTC_REF_RESISTOR));
    return (tempK - 273.15f) + tempOffset;
}

// ================== ПРИМЕНЕНИЕ ШИМ ============================
void applyFanPWM(uint8_t fan, uint8_t pwm) {
    Calibration &c = (fan == 1) ? calib1 : calib2;
    uint8_t out = pwm;
    uint8_t pin = (fan == 1) ? FAN1_PWM : FAN2_PWM;

    if (out == 0) {
        analogWrite(pin, 0);
        return;
    }
    if (out < c.stopPoint) out = c.startPoint;
    analogWrite(pin, out);
}

// =============== ПЛАВНЫЕ ДОГОНЯЛКИ ===========================
bool smoothStep(int dir) {
    if (dir == 0) return false;
    
    uint8_t maxPWM = max(fan1PWM, fan2PWM);
    uint8_t threshold = (maxPWM < PWM_50_PERCENT) ? DIFF_THRESHOLD_LOW : DIFF_THRESHOLD_HIGH;

    int diff = (int)fan1PWM - (int)fan2PWM;

    if (dir > 0) {
        if (diff < 0) {
            if (abs(diff) >= threshold) {
                fan1PWM = min(255, fan1PWM + STEP_PWM);
                applyFanPWM(1, fan1PWM);
                return true;
            } else {
                if (fan1PWM < 255) {
                    fan1PWM = min(255, fan1PWM + STEP_PWM);
                    applyFanPWM(1, fan1PWM);
                    return true;
                }
            }
        } else if (diff > 0) {
            if (diff >= threshold) {
                fan2PWM = min(255, fan2PWM + STEP_PWM);
                applyFanPWM(2, fan2PWM);
                return true;
            } else {
                if (fan1PWM < 255) {
                    fan1PWM = min(255, fan1PWM + STEP_PWM);
                    applyFanPWM(1, fan1PWM);
                    return true;
                } else if (fan2PWM < 255) {
                    fan2PWM = min(255, fan2PWM + STEP_PWM);
                    applyFanPWM(2, fan2PWM);
                    return true;
                }
            }
        } else {
            if (fan1PWM < 255) {
                fan1PWM = min(255, fan1PWM + STEP_PWM);
                applyFanPWM(1, fan1PWM);
                return true;
            }
        }
    } else {
        if (diff > 0) {
            if (diff >= threshold) {
                fan2PWM = (fan2PWM >= STEP_PWM) ? fan2PWM - STEP_PWM : 0;
                applyFanPWM(2, fan2PWM);
                return true;
            } else {
                if (fan1PWM >= STEP_PWM) {
                    fan1PWM -= STEP_PWM;
                    applyFanPWM(1, fan1PWM);
                    return true;
                }
            }
        } else if (diff < 0) {
            if (abs(diff) >= threshold) {
                fan1PWM = (fan1PWM >= STEP_PWM) ? fan1PWM - STEP_PWM : 0;
                applyFanPWM(1, fan1PWM);
                return true;
            } else {
                if (fan2PWM >= STEP_PWM) {
                    fan2PWM -= STEP_PWM;
                    applyFanPWM(2, fan2PWM);
                    return true;
                }
            }
        } else {
            if (fan1PWM >= STEP_PWM) {
                fan1PWM -= STEP_PWM;
                applyFanPWM(1, fan1PWM);
                return true;
            } else if (fan2PWM >= STEP_PWM) {
                fan2PWM -= STEP_PWM;
                applyFanPWM(2, fan2PWM);
                return true;
            }
        }
    }
    return false;
}

// ==================== ОБРАБОТЧИК РЕЖИМОВ ======================
void processNormal(float temp) {
    if (temp >= TEMP_NORMAL_START) {
        if (fan1PWM == 0 && fan2PWM == 0) {
            fan1PWM = calib1.startPoint;
            applyFanPWM(1, fan1PWM);
            startupPhase = true;
            startupTimer = millis();
            startupDropped = false;
            return;
        }
        smoothStep(1);
    } else if (temp < TEMP_NORMAL_STOP) {
        smoothStep(-1);
    }
}

void processHighway(float temp) {
    if (temp >= TEMP_HIGHWAY_START) {
        if (fan2PWM == 0 && fan1PWM == 0) {
            fan2PWM = calib2.startPoint;
            applyFanPWM(2, fan2PWM);
            startupPhase = true;
            startupTimer = millis();
            startupDropped = false;
            return;
        }
        smoothStep(1);
        if (temp >= TEMP_HIGHWAY_HOT && fan1PWM < 255) {
            if (fan1PWM < fan2PWM) {
                fan1PWM = min(255, fan1PWM + STEP_PWM);
                applyFanPWM(1, fan1PWM);
            }
        }
    } else if (temp < TEMP_NORMAL_STOP) {
        smoothStep(-1);
    }
}

void processCity(float temp) {
    if (temp >= TEMP_CITY_START) {
        if (fan1PWM == 0 && fan2PWM == 0) {
            fan1PWM = calib1.startPoint;
            fan2PWM = calib2.startPoint;
            applyFanPWM(1, fan1PWM);
            applyFanPWM(2, fan2PWM);
            startupPhase = true;
            startupTimer = millis();
            startupDropped = false;
            return;
        }
        if (temp >= TEMP_CITY_HOT) {
            for (int i = 0; i < 2; i++) {
                if (!smoothStep(1)) break;
            }
        } else {
            smoothStep(1);
        }
    } else if (temp < TEMP_NORMAL_STOP) {
        smoothStep(-1);
    }
}

// ==================== СТАРТОВАЯ ФАЗА ==========================
void handleStartup() {
    if (!startupPhase) return;
    uint32_t now = millis();
    if (!startupDropped && (now - startupTimer >= 1000)) {
        if (fan1PWM > calib1.stopPoint) {
            fan1PWM = calib1.stopPoint;
            applyFanPWM(1, fan1PWM);
        }
        if (fan2PWM > calib2.stopPoint) {
            fan2PWM = calib2.stopPoint;
            applyFanPWM(2, fan2PWM);
        }
        startupDropped = true;
    }
}

// ==================== ОСНОВНОЙ ЦИКЛ ==========================
void processFans() {
    float temp = readCoolantTemp();

    if (temp >= TEMP_OVERHEAT) {
        fan1PWM = fan2PWM = 255;
        applyFanPWM(1, 255);
        applyFanPWM(2, 255);
        startupPhase = false;
        return;
    }

    if (manualMode) {
        applyFanPWM(1, manualPWM1);
        applyFanPWM(2, manualPWM2);
        fan1PWM = manualPWM1;
        fan2PWM = manualPWM2;
        startupPhase = false;
        return;
    }

    handleStartup();

    switch (currentMode) {
        case MODE_NORMAL:  processNormal(temp); break;
        case MODE_HIGHWAY: processHighway(temp); break;
        case MODE_CITY:    processCity(temp); break;
    }

    if (startupPhase && (fan1PWM != calib1.startPoint || fan2PWM != calib2.startPoint)) {
        startupPhase = false;
    }

    updateTrafficDetection();
}

void updateTrafficDetection() {
    if (currentMode == MODE_NORMAL && !manualMode) {
        if (fan1PWM >= PWM_70_PERCENT && fan2PWM >= PWM_70_PERCENT) {
            if (!trafficTimerActive) {
                trafficTimer = millis();
                trafficTimerActive = true;
            } else if (millis() - trafficTimer >= TRAFFIC_TIMEOUT_MS) {
                currentMode = MODE_CITY;
                trafficTimerActive = false;
                modeFromScreen = false;
            }
        } else {
            trafficTimerActive = false;
        }
    } else if (currentMode == MODE_CITY && !manualMode) {
        if (fan1PWM <= PWM_30_PERCENT && fan2PWM <= PWM_30_PERCENT) {
            if (!normalTimerActive) {
                normalTimer = millis();
                normalTimerActive = true;
            } else if (millis() - normalTimer >= TRAFFIC_EXIT_MS) {
                currentMode = MODE_NORMAL;
                normalTimerActive = false;
                modeFromScreen = false;
            }
        } else {
            normalTimerActive = false;
        }
    }
}

// ==================== ОТПРАВКА ТЕЛЕМЕТРИИ =====================
void sendTelemetry() {
    int16_t temp = (int16_t)(readCoolantTemp() * 10.0f);
    uint8_t payload[4];
    payload[0] = fan1PWM;
    payload[1] = fan2PWM;
    payload[2] = temp & 0xFF;
    payload[3] = (temp >> 8) & 0xFF;

    uint8_t frame[FRAME_TOTAL_SIZE];
    frame[0] = FRAME_MAGIC;
    frame[1] = MSG_FAN_TELEMETRY;
    frame[2] = ADDR_ARDUINO;
    frame[3] = ADDR_ESP32_P4;
    frame[4] = 4;
    frame[5] = payload[0];
    frame[6] = payload[1];
    frame[7] = payload[2];
    frame[8] = payload[3];
    frame[9] = crc8(frame, 9);
    softSerial.write(frame, FRAME_TOTAL_SIZE);
}

// ==================== HEARTBEAT ===============================
void sendHeartbeat() {
    int16_t temp = (int16_t)(readCoolantTemp() * 10.0f);
    uint8_t payload[4];
    payload[0] = fan1PWM;
    payload[1] = fan2PWM;
    payload[2] = temp & 0xFF;
    payload[3] = (temp >> 8) & 0xFF;

    uint8_t frame[FRAME_TOTAL_SIZE];
    frame[0] = FRAME_MAGIC;
    frame[1] = MSG_HEARTBEAT_RSP;
    frame[2] = ADDR_ARDUINO;
    frame[3] = ADDR_ESP32_P4;
    frame[4] = 4;
    frame[5] = payload[0];
    frame[6] = payload[1];
    frame[7] = payload[2];
    frame[8] = payload[3];
    frame[9] = crc8(frame, 9);
    softSerial.write(frame, FRAME_TOTAL_SIZE);
}

// ==================== ПРИЁМ UART КОМАНД ======================
void handleUARTCommand(uint8_t msg_id, uint8_t* payload, uint8_t len) {
    lastRxTime = millis();  // отметка времени приёма

    switch (msg_id) {
        case MSG_FAN_SET_MODE:
            if (len >= 1 && payload[0] >= 1 && payload[0] <= 3) {
                currentMode = (FanMode)payload[0];
                manualMode = false;
                modeFromScreen = true;
            }
            break;
        case MSG_FAN_SET_PWM1:
            if (len >= 1) { manualPWM1 = payload[0]; manualMode = true; }
            break;
        case MSG_FAN_SET_PWM2:
            if (len >= 1) { manualPWM2 = payload[0]; manualMode = true; }
            break;
        case MSG_FAN_SET_AUTO:
            manualMode = false;
            modeFromScreen = false;
            break;
        case MSG_FAN_TELEMETRY:
            sendTelemetry();
            break;
        case MSG_FAN_CALIB_START_POINT:
            if (len >= 1) { calib1.startPoint = payload[0]; calib2.startPoint = payload[0]; }
            break;
        case MSG_FAN_CALIB_STOP_POINT:
            if (len >= 1) { calib1.stopPoint = payload[0]; calib2.stopPoint = payload[0]; }
            break;
        case MSG_FAN_CALIB_NOISE_LOW:
            if (len >= 1) { calib1.noiseLow = payload[0]; calib2.noiseLow = payload[0]; }
            break;
        case MSG_FAN_CALIB_NOISE_HIGH:
            if (len >= 1) { calib1.noiseHigh = payload[0]; calib2.noiseHigh = payload[0]; }
            break;
        case MSG_FAN_CALIB_SAVE:
            saveCalibration();
            break;
        case MSG_TEMP_OFFSET_SET:
            if (len >= 2) {
                int16_t offsetRaw = payload[0] | (payload[1] << 8);
                tempOffset = offsetRaw / 10.0f;
                saveCalibration();
            }
            break;
        case MSG_TEMP_OFFSET_GET: {
            int16_t offsetRaw = (int16_t)(tempOffset * 10.0f);
            uint8_t resp[2];
            resp[0] = offsetRaw & 0xFF;
            resp[1] = (offsetRaw >> 8) & 0xFF;
            uint8_t frame[FRAME_TOTAL_SIZE];
            frame[0] = FRAME_MAGIC;
            frame[1] = MSG_TEMP_OFFSET_GET;
            frame[2] = ADDR_ARDUINO;
            frame[3] = ADDR_ESP32_P4;
            frame[4] = 2;
            frame[5] = resp[0];
            frame[6] = resp[1];
            frame[7] = 0;
            frame[8] = 0;
            frame[9] = crc8(frame, 9);
            softSerial.write(frame, FRAME_TOTAL_SIZE);
            break;
        }
    }
}

void checkUART() {
    while (softSerial.available()) {
        uint8_t b = softSerial.read();
        if (!frameStarted) {
            if (b == FRAME_MAGIC) {
                rxBuffer[0] = b;
                rxIndex = 1;
                frameStarted = true;
            }
        } else {
            rxBuffer[rxIndex++] = b;
            if (rxIndex >= FRAME_TOTAL_SIZE) {
                frameStarted = false;
                uint8_t expectedCRC = crc8(rxBuffer, 9);
                if (expectedCRC == rxBuffer[9]) {
                    uint8_t msg_id = rxBuffer[1];
                    uint8_t dst = rxBuffer[3];
                    uint8_t len = rxBuffer[4];
                    if (dst == ADDR_ARDUINO || dst == 0x00) {
                        handleUARTCommand(msg_id, &rxBuffer[5], len);
                    }
                }
            }
        }
    }
}

// =================== LCD ОБНОВЛЕНИЕ ===========================
void updateLCD() {
    float tempRaw = readCoolantTemp() - tempOffset; // сырая
    float tempCorrected = readCoolantTemp();          // с учётом offset

    const char* modeStr = "NORM";
    if (manualMode) modeStr = "MAN";
    else if (currentMode == MODE_HIGHWAY) modeStr = "HWY";
    else if (currentMode == MODE_CITY) modeStr = "CITY";

    bool linkOk = (millis() - lastRxTime < 6000);
    const char* linkStr = linkOk ? "OK" : "NO";
    const char* screenPrefix = modeFromScreen ? ">>" : "  ";

    char line0[17], line1[17];
    snprintf(line0, sizeof(line0), "@:%03d t%c%03d %s%s",
             fan1PWM, 0xDF, (int)round(tempRaw), linkStr, screenPrefix);
    snprintf(line1, sizeof(line1), "@:%03d t%c%03d %s",
             fan2PWM, 0xDF, (int)round(tempCorrected), modeStr);

    String newLine0 = line0;
    String newLine1 = line1;

    if (newLine0 != lcdLine0) {
        lcd.setCursor(0, 0);
        lcd.print(newLine0);
        lcdLine0 = newLine0;
    }
    if (newLine1 != lcdLine1) {
        lcd.setCursor(0, 1);
        lcd.print(newLine1);
        lcdLine1 = newLine1;
    }
}

// =================== ТЕСТОВЫЙ РЕЖИМ ==========================
void enterTestMode() {
    testMode = true;
    testTempValue = 90.0;
    Serial.begin(115200);
    Serial.println("=== TEST MODE ===");
    Serial.println("Enter temperature in Celsius (e.g. 100.5) and press Enter.");
    Serial.println("Current PWM will be printed every second.");
    Serial.println("=================================");
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    uint32_t lastPrintTime = 0;
    uint32_t lastProcessTime = 0;
    String inputBuffer = "";
    
    while (testMode) {
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                if (inputBuffer.length() > 0) {
                    float newTemp = inputBuffer.toFloat();
                    testTempValue = newTemp;
                    Serial.print("New temp set to: ");
                    Serial.println(testTempValue);
                    inputBuffer = "";
                }
            } else {
                inputBuffer += c;
            }
        }

        if (millis() - lastProcessTime >= PROCESS_INTERVAL) {
            lastProcessTime = millis();
            processFans();
        }

        if (millis() - lastPrintTime >= 1000) {
            lastPrintTime = millis();
            Serial.print("Temp: ");
            Serial.print(testTempValue);
            Serial.print(" C, Fan1 PWM: ");
            Serial.print(fan1PWM);
            Serial.print(", Fan2 PWM: ");
            Serial.println(fan2PWM);
        }
    }
    // После выхода из тестового режима
    digitalWrite(LED_BUILTIN, LOW);
    testMode = false;
    // Переинициализируем SoftwareSerial и LCD
    softSerial.begin(UART_BAUD);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcdLine0 = "";
    lcdLine1 = "";
}

// ========================= SETUP =============================
void setup() {
    pinMode(FAN1_PWM, OUTPUT);
    pinMode(FAN2_PWM, OUTPUT);
    analogWrite(FAN1_PWM, 0);
    analogWrite(FAN2_PWM, 0);

    // Таймер 1 -> 31.25 кГц (пины 9,10)
    TCCR1B = (TCCR1B & 0b11111000) | 0x01;

    pinMode(TEST_MODE_PIN, INPUT_PULLUP);
    
    loadCalibration();

    // Инициализация LCD
    lcd.init();
    lcd.backlight();
    
    if (digitalRead(TEST_MODE_PIN) == LOW) {
        enterTestMode();
        // После тестового режима продолжаем нормальную инициализацию
    }
    
    softSerial.begin(UART_BAUD);
    Serial.begin(115200);
    Serial.println("Radiator Fan Controller started.");
}

void loop() {
    static uint32_t lastProcess = 0;
    static uint32_t lastHeartbeat = 0;
    static uint32_t lastLCD = 0;

    if (millis() - lastProcess >= PROCESS_INTERVAL) {
        lastProcess = millis();
        processFans();
    }
    
    checkUART();

    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        lastHeartbeat = millis();
        sendHeartbeat();
    }

    if (millis() - lastLCD >= LCD_INTERVAL) {
        lastLCD = millis();
        updateLCD();
    }
}
