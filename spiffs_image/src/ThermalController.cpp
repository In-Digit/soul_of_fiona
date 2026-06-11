#include "ThermalController.h"

extern CarData carData;

ThermalController::ThermalController(CarData& data)
    : _carData(data), _lastProcessTime(0), _integral(0), _prevError(0),
      _prevOutput1(0), _prevOutput2(0), _lastCoolingTime(0),
      _tempBufIndex(0), _lastClimateTime(0), _climateState(IDLE),
      _stablePWM(0), _learningStep(25), _learningDirection(0), _basePWM(0),
      _learningStartTime(0), _holdCheckTime(0),
      _calibActive(false), _calibFanIndex(0), _calibPWM(0),
      _currentHeaterDuty(0), _sensorFault(false), _engineRunningSince(0),
      _startPhase(false), _startPhaseBegin(0)
{
    memset(_tempBuffer, 0, sizeof(_tempBuffer));
}

void ThermalController::begin() {
    pinMode(ARDUINO_PRESENT_PIN, INPUT_PULLUP);
    updateArduinoPresence();

    // ШИМ-канал печки: 20 кГц, 10 бит (0–1023)
    // Частота выше порога слышимости, 10 бит дают максимальную гибкость регулирования.
    ledcSetup(2, 20000, 10);
    ledcAttachPin(HEATER_PWM_PIN, 2);
    ledcWrite(2, 0);

    if (_carData.fanControlEnabled) {
        ledcSetup(0, 25000, 10); ledcAttachPin(FAN_PWM_PIN1, 0);
        ledcSetup(1, 25000, 10); ledcAttachPin(FAN_PWM_PIN2, 1);
        ledcWrite(0, 0); ledcWrite(1, 0);
    }

    // Параметры смещённой шкалы (дефолты)
    _heaterStartPWM = 230;   // уверенный старт с места
    _heaterStopPWM  = 185;   // минимальные стабильные обороты после старта

    // Пересчёт в единицы duty (0–1023)
    _heaterStopDuty  = (uint16_t)(_heaterStopPWM * 1023UL / 255);
    _heaterStartDuty = (uint16_t)(_heaterStartPWM * 1023UL / 255);
    _stepPerUnit     = (1023.0f - _heaterStopDuty) / 255.0f;  // шаг на одно деление экрана (0–255)

    _currentHeaterDuty = 0;
    _sensorFault = false;
    _startPhase = false;
}

void ThermalController::process() {
    updateArduinoPresence();
    uint32_t now = millis();
    if (now - _lastProcessTime < 500) return;
    _lastProcessTime = now;

    // Отслеживание работы двигателя для задержки 5 секунд
    if (_carData.rpmValue > 650) {
        if (_engineRunningSince == 0) {
            _engineRunningSince = now;
        }
    } else {
        _engineRunningSince = 0;
    }

    processClimate();   // печка работает всегда
    if (_carData.fanControlEnabled) {
        processCooling();   // вентиляторы радиатора, если активированы
    }
}

// ----------------------- ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ---------------------------

void ThermalController::updateArduinoPresence() {
    bool present = (digitalRead(ARDUINO_PRESENT_PIN) == LOW);
    if (present != _carData.arduinoPresent) {
        _carData.arduinoPresent = present;
        if (present && _carData.fanControlEnabled) {
            ledcWrite(0, 0); ledcWrite(1, 0);
            _carData.fanControlEnabled = false;
            Serial.println("[Thermal] Arduino detected, fan control disabled");
        } else if (!present) {
            Serial.println("[Thermal] Arduino lost, but not taking over (wait for command)");
        }
    }
}

float ThermalController::readNTC(uint8_t pin) const {
    long sum = 0;
    for (int i = 0; i < 8; i++) sum += analogRead(pin);
    float raw = sum / 8.0f;
    if (raw < 1) return -100; // обрыв
    float vout = raw * 3.3f / 4095.0f;
    float r = 10000.0f * ((3.3f / vout) - 1.0f);
    float t = 1.0f / ((1.0f/298.15f) + (1.0f/3435.0f) * log(r/10000.0f)) - 273.15f;
    return t;
}

float ThermalController::getCorrectedCoolantTemp() const {
    return _carData.tempValue + _carData.coolantTempOffset;
}

bool ThermalController::isCabinSensorOK() {
    float t = readNTC(NTC_CABIN_PIN);
    if (t < -50 || t > 100) {
        if (!_sensorFault) {
            Serial.printf("[Thermal] Cabin sensor fault! Value: %.1f°C\n", t);
            _sensorFault = true;
        }
        return false;
    }
    _sensorFault = false;
    return true;
}

// Прямое управление ШИМ (duty 0–1023) без какой-либо коррекции
void ThermalController::applyHeaterPWM(uint16_t duty) {
    if (duty == 0) {
        ledcWrite(2, 0);
        _carData.climateCurrentPWM = 0;
        return;
    }
    if (duty > 1023) duty = 1023;
    ledcWrite(2, duty);
    // Обратное преобразование для отображения (0–255)
    _carData.climateCurrentPWM = (uint8_t)(duty * 255UL / 1023);
}

// Плавное изменение с рампой и смещённой шкалой
void ThermalController::applyHeaterPWM_Ramp(uint8_t targetPWM) {
    uint32_t now = millis();

    // Целевое значение в duty с учётом смещённой шкалы
    uint16_t targetDuty;
    if (targetPWM == 0) {
        targetDuty = 0;
    } else {
        // Шкала начинается от _heaterStopDuty
        targetDuty = (uint16_t)(_heaterStopDuty + _stepPerUnit * targetPWM);
        if (targetDuty > 1023) targetDuty = 1023;
    }

    // Если вентилятор был полностью выключен и пришла команда на запуск
    if (_currentHeaterDuty == 0 && targetPWM > 0) {
        // Стартовый импульс: сразу даём уверенный старт
        _currentHeaterDuty = (float)_heaterStartDuty;
        _startPhase = true;
        _startPhaseBegin = now;
        applyHeaterPWM(_heaterStartDuty);
        return;
    }

    // Если идёт фаза стартового импульса (держим start 1 секунду)
    if (_startPhase) {
        if (now - _startPhaseBegin < 1000) {
            // Удерживаем стартовые обороты, не обрабатываем target
            return;
        } else {
            _startPhase = false;
            // По истечении 1 секунды начинаем плавное снижение к целевой duty
            // _currentHeaterDuty всё ещё равно _heaterStartDuty, рампа начнёт уменьшать
        }
    }

    // Плавное изменение current -> target с ограничением по скорости
    float current = _currentHeaterDuty;
    float target = targetDuty;

    // Скорость изменения: разгон 0–1023 за 2 с, торможение за 3 с
    // Шаг за 500 мс: разгон 1023*0.5*0.5 ≈ 255.75, торможение 1023*0.33*0.5 ≈ 168.8
    float stepUp = 1023.0f * 0.5f * 0.5f;
    float stepDown = 1023.0f * 0.33f * 0.5f;

    if (target > current) {
        current += stepUp;
        if (current > target) current = target;
    } else if (target < current) {
        current -= stepDown;
        if (current < target) current = target;
    }

    _currentHeaterDuty = current;
    applyHeaterPWM((uint16_t)current);
}

// ----------------------- ОХЛАЖДЕНИЕ (без изменений) -----------------------
void ThermalController::processCooling() {
    if (_calibActive) return;

    float temp = getCorrectedCoolantTemp();
    if (temp < 0) return;

    _tempBuffer[_tempBufIndex] = temp;
    _tempBufIndex = (_tempBufIndex + 1) % TEMP_BUF_SIZE;

    float setpoint = _carData.fanSetpoint;
    float error = temp - setpoint;

    float P = _carData.fanKp * error;
    _integral += error * 0.5f;
    if (_integral > 500) _integral = 500;
    if (_integral < -500) _integral = -500;
    float I = _carData.fanKi * _integral;
    float D = _carData.fanKd * getCoolantTrend();

    float output = P + I + D;
    if (output > 510) output = 510;
    if (output < 0) output = 0;

    static float p1 = 0, p2 = 0;
    float target = output;
    if (target > p1 + p2) {
        if (p1 <= p2) p1 = min(p1 + 10.0f, 255.0f);
        else p2 = min(p2 + 10.0f, 255.0f);
    } else if (target < p1 + p2) {
        if (p1 >= p2) p1 = max(p1 - 10.0f, 0.0f);
        else p2 = max(p2 - 10.0f, 0.0f);
    }
    uint8_t pwm1 = (uint8_t)p1;
    uint8_t pwm2 = (uint8_t)p2;

    applyFanPWM(1, pwm1);
    applyFanPWM(2, pwm2);
    _carData.fanCurrentPWM1 = (pwm1 * 100 + 127) / 255;
    _carData.fanCurrentPWM2 = (pwm2 * 100 + 127) / 255;
}

// ----------------------- КЛИМАТ (печка) ------------------------------------
void ThermalController::processClimate() {
    if (_calibActive) return;

    // Двигатель должен работать не менее 5 секунд
    bool engineActive = (_engineRunningSince != 0) && (millis() - _engineRunningSince >= 5000);
    if (!engineActive) {
        applyHeaterPWM_Ramp(0);
        return;
    }

    float coolantTemp = getCorrectedCoolantTemp();

    // Аварийный режим при перегреве
    if (coolantTemp >= 115.0f) {
        applyHeaterPWM_Ramp(255);
        return;
    }

    bool sensorOK = isCabinSensorOK();
    if (!sensorOK) {
        if (!_carData.climateAutoMode) {
            applyHeaterPWM_Ramp(_carData.climateManualPWM);
        } else {
            applyHeaterPWM_Ramp(77); // вентиляция 30%
        }
        return;
    }

    float cabinTemp = readNTC(NTC_CABIN_PIN);
    _carData.ntcCabin = cabinTemp;

    uint8_t targetPWM = 0;

    if (!_carData.climateAutoMode) {
        targetPWM = _carData.climateManualPWM;
    } else {
        switch (_carData.climatePreset) {
            case 2: { // Стекло
                if (cabinTemp < 3.0f && coolantTemp < 70.0f) {
                    targetPWM = 102;   // 40%
                } else {
                    targetPWM = 255;
                }
                break;
            }
            case 3: // Вентиляция
                targetPWM = 77;
                break;
            case 1:
            default: { // Авто (асимметричный)
                float error = _carData.climateSetpoint - cabinTemp;
                if (error > 2.0f) {
                    if (coolantTemp >= 70.0f) {
                        targetPWM = constrain(50 + (int)((error - 2.0f) * 30.0f), 50, 204);
                    } else {
                        targetPWM = 0;
                    }
                } else if (error < -5.0f) {
                    float deltaCool = cabinTemp - _carData.climateSetpoint;
                    targetPWM = constrain(50 + (int)((deltaCool - 5.0f) * 50.0f), 50, 255);
                } else {
                    targetPWM = 0;
                }
                break;
            }
        }
    }

    applyHeaterPWM_Ramp(targetPWM);
}

// ----------------------- ГЕТТЕРЫ ДЛЯ UART --------------------------
uint8_t ThermalController::getFanPWM1() const { return _carData.fanCurrentPWM1; }
uint8_t ThermalController::getFanPWM2() const { return _carData.fanCurrentPWM2; }
uint8_t ThermalController::getFanMode() const { return _carData.fanMode; }
bool    ThermalController::getFanAuto() const { return _carData.fanAutoMode; }
float   ThermalController::getCoolantTemp() const { return getCorrectedCoolantTemp(); }
uint8_t ThermalController::getClimatePWM() const { return _carData.climateCurrentPWM; }
uint8_t ThermalController::getClimatePreset() const { return _carData.climatePreset; }
bool    ThermalController::getClimateAuto() const { return _carData.climateAutoMode; }
float   ThermalController::getCabinTemp() const { return _carData.ntcCabin; }

// ----------------------- КАЛИБРОВКИ ВЕНТИЛЯТОРОВ ---------------------------
void ThermalController::startFanCalib(uint8_t fanIndex) {
    _calibActive = true;
    _calibFanIndex = fanIndex;
    _calibPWM = 0;
    applyFanPWM(fanIndex, 0);
    Serial.printf("[Calib] Fan %d calibration started\n", fanIndex);
}

void ThermalController::stepFanCalib(int8_t step) {
    if (!_calibActive || _calibFanIndex == 3) return;
    _calibPWM = constrain(_calibPWM + step, 0, 255);
    applyFanPWM(_calibFanIndex, _calibPWM);
    Serial.printf("[Calib] Fan PWM=%d\n", _calibPWM);
}

void ThermalController::saveFanCalibPoint(uint8_t pointType) {
    if (!_calibActive || _calibFanIndex == 3) return;
    if (_calibFanIndex == 1) {
        switch (pointType) {
            case 0: _carData.fan1CalibStartPoint = _calibPWM; break;
            case 1: _carData.fan1CalibStopPoint  = _calibPWM; break;
            case 2: _carData.fan1NoiseLow       = _calibPWM; break;
            case 3: _carData.fan1NoiseHigh      = _calibPWM; break;
        }
    } else {
        switch (pointType) {
            case 0: _carData.fan2CalibStartPoint = _calibPWM; break;
            case 1: _carData.fan2CalibStopPoint  = _calibPWM; break;
            case 2: _carData.fan2NoiseLow       = _calibPWM; break;
            case 3: _carData.fan2NoiseHigh      = _calibPWM; break;
        }
    }
    Serial.printf("[Calib] Point %d saved: %d\n", pointType, _calibPWM);
}

void ThermalController::saveFanCalib() {
    _calibActive = false;
    applyFanPWM(_calibFanIndex, 0);
    Serial.println("[Calib] Fan calibration saved, returning to normal mode");
}

// ----------------------- КАЛИБРОВКИ ПЕЧКИ ---------------------------
void ThermalController::startHeaterCalib() {
    _calibActive = true;
    _calibFanIndex = 3;
    _calibPWM = 0;
    applyHeaterPWM((uint16_t)0);   // явное приведение
    Serial.println("[Calib] Heater calibration started");
}

void ThermalController::stepHeaterCalib(int8_t step) {
    if (!_calibActive || _calibFanIndex != 3) return;
    _calibPWM = constrain(_calibPWM + step, 0, 255);
    applyHeaterPWM((uint16_t)(_calibPWM * 1023UL / 255));   // конвертируем в duty
    Serial.printf("[Calib] Heater PWM=%d\n", _calibPWM);
}

void ThermalController::saveHeaterCalibPoint(uint8_t pointType) {
    if (!_calibActive || _calibFanIndex != 3) return;
    switch (pointType) {
        case 0: _carData.climateCalibStartPoint = _calibPWM; break;
        case 1: _carData.climateCalibStopPoint  = _calibPWM; break;
        case 2: _carData.climateCalibNoiseLow   = _calibPWM; break;
        case 3: _carData.climateCalibNoiseHigh  = _calibPWM; break;
    }
    Serial.printf("[Calib] Heater point %d saved: %d\n", pointType, _calibPWM);
}

void ThermalController::saveHeaterCalib() {
    _calibActive = false;
    applyHeaterPWM((uint16_t)0);   // явное приведение
    Serial.println("[Calib] Heater calibration saved, returning to normal mode");
}

// ----------------------- ПРИМЕНЕНИЕ ШИМ ВЕНТИЛЯТОРОВ -----------------------
void ThermalController::applyFanPWM(uint8_t fan, uint8_t pwm) {
    uint8_t startPoint = (fan == 1) ? _carData.fan1CalibStartPoint : _carData.fan2CalibStartPoint;
    uint8_t stopPoint  = (fan == 1) ? _carData.fan1CalibStopPoint  : _carData.fan2CalibStopPoint;
    uint8_t noiseLow   = (fan == 1) ? _carData.fan1NoiseLow       : _carData.fan2NoiseLow;
    uint8_t noiseHigh  = (fan == 1) ? _carData.fan1NoiseHigh      : _carData.fan2NoiseHigh;

    if (pwm == 0) { ledcWrite(fan - 1, 0); return; }
    if (startPoint > 0 && stopPoint > 0 && pwm < stopPoint) pwm = startPoint;
    if (noiseLow > 0 && pwm < noiseLow) pwm = noiseLow;
    if (noiseHigh > 0 && pwm < noiseHigh) pwm = noiseHigh;

    uint32_t duty = (uint32_t)(pwm * 1023.0f / 255.0f + 0.5f);
    if (duty > 1023) duty = 1023;
    ledcWrite(fan - 1, duty);
}

// Тренд температуры ОЖ для PID
float ThermalController::getCoolantTrend() const {
    if (_tempBufIndex < 5) return 0;
    float sum1 = 0, sum2 = 0;
    int half = TEMP_BUF_SIZE / 2;
    for (int i = 0; i < half; i++) {
        sum1 += _tempBuffer[(_tempBufIndex - TEMP_BUF_SIZE + i + TEMP_BUF_SIZE) % TEMP_BUF_SIZE];
        sum2 += _tempBuffer[(_tempBufIndex - half + i + TEMP_BUF_SIZE) % TEMP_BUF_SIZE];
    }
    return (sum2 - sum1) / (half * 5.0f);
}
float ThermalController::getClimateSetpoint() const {
    return _carData.climateSetpoint;
}
// ----------------------- UART-обработчики (без изменений) --------------------
void ThermalController::setFanMode(uint8_t mode) { _carData.fanMode = mode; }
void ThermalController::setFanPWM1(uint8_t pwm) { _carData.fanManualPWM = pwm; _carData.fanAutoMode = false; }
void ThermalController::setFanPWM2(uint8_t pwm) { }
void ThermalController::setFanAuto(bool autoMode) { _carData.fanAutoMode = autoMode; }
void ThermalController::setClimatePreset(uint8_t preset) { _carData.climatePreset = preset; }
void ThermalController::setClimateSetpoint(float temp) { _carData.climateSetpoint = temp; _carData.climateAutoMode = false; }
void ThermalController::setClimatePWM(uint8_t pwm) { _carData.climateManualPWM = pwm; _carData.climateAutoMode = false; }
void ThermalController::setClimateAuto(bool autoMode) { _carData.climateAutoMode = autoMode; }