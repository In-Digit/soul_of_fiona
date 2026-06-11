#ifndef TRIP_MANAGER_H
#define TRIP_MANAGER_H

#include <Arduino.h>
#include "CarData.h"
#include "OBDEngine.h"

class TripManager {
public:
    TripManager(CarData& data, OBDEngine& obd);
    void begin();
    void process();
    void toggleTrip();

    void setTimeProvider(unsigned long (*provider)()) { _timeProvider = provider; }
    void clearDriveCycles();

private:
    CarData& _carData;
    OBDEngine& _obd;
    uint32_t _lastProcessTime;
    uint32_t _engineOffSince;
    enum EngineState { ENGINE_UNKNOWN, ENGINE_RUNNING, ENGINE_OFF };
    EngineState _engineState;

    // Текущая поездка (рейс) — для подсчёта пробега и топлива
    float _distanceThisTripKm;
    float _fuelUsedThisTripL;
    float _lastRangeValue;

    // Состояние текущего заезда (drive cycle)
    bool _driveActive;
    uint32_t _driveStartTime;
    uint32_t _driveEndTime;
    uint32_t _driveDuration;
    uint32_t _driveDistance;    // метры
    float    _driveFuelUsedL;
    uint16_t _driveMaxSpeed;
    uint16_t _driveMaxLPH;
    uint32_t _driveAvgThrottleAccum;
    uint16_t _driveThrottleSamples;
    uint16_t _driveMaxThrottleRel;
    uint32_t _driveWarmupSeconds;
    uint16_t _driveAggressiveCount;
    uint16_t _driveFullThrottleCount;
    bool     _driveMoved;       // было движение в этом заезде?
    uint32_t _driveMoveSeconds;
    float    _drivePrevThrottleRel;

    unsigned long (*_timeProvider)();

    void updateEngineState();
    void updateTripLogic();
    void integrateFuelAndCost();
    void checkAutoStop();
    void updateRange();

    void finalizeTripStats(bool isManual);
    void resetTripCounters();
    void updateDayStats(uint32_t nowUnix);

    // Заезды
    void startDriveIfNeeded();
    void updateDrive();
    void finalizeDrive();
};

#endif