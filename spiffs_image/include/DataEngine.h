#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

#include <Arduino.h>
#include "OBDEngine.h"
#include "CarData.h"

struct PidRecord {
    const char* command;
    uint16_t interval;
    uint8_t priority;
    uint32_t lastRequest;
    void (*updateCarData)(CarData* data, float value);
};

class DataEngine {
public:
    DataEngine(OBDEngine& obd, CarData& data);
    void begin();
    void process();

    static void updateMAF(CarData* data, float value);
    static void updateRPM(CarData* data, float value);
    static void updateSpeed(CarData* data, float value);
    static void updateCoolantTemp(CarData* data, float value);
    static void updateVoltage(CarData* data, float value);
    static void updateThrottle(CarData* data, float value);
    static void updateThrottleRel(CarData* data, float value);

    static void updateLight();
    static uint8_t getLightLevel();
    static uint16_t getLightRaw();

private:
    OBDEngine& _obd;
    CarData& _carData;
    static const int PID_COUNT = 7;
    PidRecord _pids[PID_COUNT];

public:
    // Геттер для профилировщика
    const PidRecord* getPids() const { return _pids; }
    int getPidCount() const { return PID_COUNT; }
};

#endif