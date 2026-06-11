#ifndef THERMAL_CONTROLLER_H
#define THERMAL_CONTROLLER_H

#include <Arduino.h>
#include "CarData.h"

#define FAN_PWM_PIN1          14
#define FAN_PWM_PIN2          27
#define HEATER_PWM_PIN        26
#define NTC_HEATER_OUT_PIN    33
#define NTC_CABIN_PIN         32
#define NTC_COOLANT_PIN       35   // не используется для управления, оставлен для совместимости
#define ARDUINO_PRESENT_PIN   25   // LOW = Arduino в сети, HIGH = нет

class ThermalController {
public:
    ThermalController(CarData& data);
    void begin();
    void process();   // вызывать каждые 500 мс

    // Методы для обработки команд UART
    void setFanMode(uint8_t mode);
    void setFanPWM1(uint8_t pwm);
    void setFanPWM2(uint8_t pwm);
    void setFanAuto(bool autoMode);
    void setClimatePreset(uint8_t preset);
    void setClimateSetpoint(float temp);
    void setClimatePWM(uint8_t pwm);      // 0-255
    void setClimateAuto(bool autoMode);

    // Калибровка вентиляторов радиатора
    void startFanCalib(uint8_t fanIndex);
    void stepFanCalib(int8_t step);
    void saveFanCalibPoint(uint8_t pointType);
    void saveFanCalib();

    // Калибровка печки
    void startHeaterCalib();
    void stepHeaterCalib(int8_t step);
    void saveHeaterCalibPoint(uint8_t pointType);
    void saveHeaterCalib();

    // Геттеры для UART-ответов
    uint8_t getFanPWM1() const;
    uint8_t getFanPWM2() const;
    uint8_t getFanMode() const;
    bool    getFanAuto() const;
    float   getCoolantTemp() const;
    uint8_t getClimatePWM() const;
    uint8_t getClimatePreset() const;
    bool    getClimateAuto() const;
    float   getCabinTemp() const;
    float   getClimateSetpoint() const;   // текущая желаемая температура (уставка)

private:
    CarData& _carData;
    uint32_t _lastProcessTime;

    // --- Охлаждение (спящее) ---
    float _integral;
    float _prevError;
    float _prevOutput1, _prevOutput2;
    uint32_t _lastCoolingTime;

    static const int TEMP_BUF_SIZE = 20;
    float _tempBuffer[TEMP_BUF_SIZE];
    int _tempBufIndex;

    // --- Климат ---
    uint32_t _lastClimateTime;
    enum ClimateState { IDLE, HEATING, LEARNING, HOLDING, DEFROST, VENT, EMERGENCY };
    ClimateState _climateState;
    float _stablePWM;
    int   _learningStep;
    int   _learningDirection;
    float _basePWM;
    uint32_t _learningStartTime;
    uint32_t _holdCheckTime;

    // --- Калибровка ---
    bool _calibActive;
    uint8_t _calibFanIndex;    // 1,2 – радиаторные, 3 – печка
    uint8_t _calibPWM;

    // --- Плавное управление печкой ---
    float _currentHeaterPWM;   // устаревшее, оставлено для совместимости
    float _currentHeaterDuty;  // текущее значение duty (0..1023) для рампы
    bool  _sensorFault;        // флаг отказа датчика салона
    uint32_t _engineRunningSince; // момент, когда RPM превысил 650 (0 = заглушен)

    // Новая логика смещённой шкалы
    uint8_t  _heaterStartPWM;   // точка уверенного старта (0-255)
    uint8_t  _heaterStopPWM;    // точка устойчивого вращения (0-255)
    uint16_t _heaterStopDuty;   // stop в единицах duty (0-1023)
    uint16_t _heaterStartDuty;  // start в единицах duty (0-1023)
    float    _stepPerUnit;      // шаг duty на одно деление экрана
    bool     _startPhase;       // фаза стартового импульса
    uint32_t _startPhaseBegin;  // момент начала импульса

    // --- Вспомогательные методы ---
    void updateArduinoPresence();
    void processCooling();
    void processClimate();
    float readNTC(uint8_t pin) const;
    float getCoolantTrend() const;
    void applyFanPWM(uint8_t fan, uint8_t pwm);
    void applyHeaterPWM(uint16_t duty);              // прямая установка ШИМ в единицах duty (0-1023)
    void applyHeaterPWM_Ramp(uint8_t targetPWM);     // плавное изменение с учётом смещённой шкалы
    bool isCabinSensorOK();
    float getCorrectedCoolantTemp() const;
};

#endif