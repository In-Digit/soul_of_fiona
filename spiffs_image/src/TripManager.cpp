#include "TripManager.h"
#include "StorageManager.h"

extern StorageManager storageManager;
extern OBDEngine obdEngine;
extern void sendTripStats();
extern void sendDayStats();
extern void sendDriveCycles();

TripManager::TripManager(CarData& data, OBDEngine& obd)
    : _carData(data), _obd(obd), _lastProcessTime(0), _engineOffSince(0),
      _engineState(ENGINE_UNKNOWN), _distanceThisTripKm(0), _fuelUsedThisTripL(0),
      _lastRangeValue(0), _driveActive(false), _driveMoved(false),
      _driveMoveSeconds(0), _drivePrevThrottleRel(0.0f), _timeProvider(nullptr)
{
}

void TripManager::begin() {
    _lastProcessTime = millis();
    _distanceThisTripKm = 0;
    _fuelUsedThisTripL = 0;
    _lastRangeValue = _carData.rangeValue;
    _driveActive = false;
    _driveMoved = false;
    _driveMoveSeconds = 0;
}

void TripManager::process() {
    uint32_t now = millis();
    if (now - _lastProcessTime < 1000) return;
    _lastProcessTime = now;

    updateEngineState();
    updateTripLogic();
    integrateFuelAndCost();
    checkAutoStop();
    updateDrive();
    startDriveIfNeeded();

    // Обновление суточной статистики только если время синхронизировано
    if (_carData.systemTime > 1577836800) {  // время после 2020 года
        updateDayStats(_carData.systemTime);
    }
}

void TripManager::updateEngineState() {
    bool engineRunning = (_carData.rpmValue > 100);
    EngineState newState = engineRunning ? ENGINE_RUNNING : ENGINE_OFF;

    if (newState != _engineState) {
        if (newState == ENGINE_OFF) {
            _engineOffSince = millis();
            if (_carData.tripState) {
                _carData.tripPauseCounter++;
            }
            _carData.dayLastStop = _carData.systemTime;
            if (!obdEngine.isEmulationMode()) {
                storageManager.requestStateSave();
            }
        } else {
            _engineOffSince = 0;
            if (_carData.dayFirstStart == 0 && _carData.systemTime > 0) {
                _carData.dayFirstStart = _carData.systemTime;
            }
        }
        _engineState = newState;
    }
}

void TripManager::updateTripLogic() {
    // Автоматическая поездка: старт при первом запуске двигателя после простоя
    if (_carData.rpmValue > 650 && !_carData.tripState) {
        bool canAutoStart = false;
        unsigned long nowUnix = (_timeProvider) ? _timeProvider() : 0;

        if (_carData.lastManualStopTime == 0) {
            // Ни разу не было ручного останова – можно стартовать сразу
            canAutoStart = true;
        } else if (nowUnix == 0) {
            // Время не синхронизировано, но ручной останов был — на всякий случай разрешаем, чтобы не блокировать систему
            canAutoStart = true;
        } else {
            // Если прошло больше 0 секунд? Мы же решили, что автостарт разрешён всегда после ручного стопа.
            canAutoStart = true;
        }

        if (canAutoStart) {
            resetTripCounters();
            _carData.tripState = true;
            _carData.tripStateDirty = true;
            _carData.tripStartTime = nowUnix;
            Serial.println("[Trip] Auto-started");
            if (!obdEngine.isEmulationMode()) {
                storageManager.requestStateSave();
            }
        }
    }
}

void TripManager::toggleTrip() {
    if (!_carData.tripState) {
        // Ручной старт
        resetTripCounters();
        _carData.tripState = true;
        _carData.tripStateDirty = true;
        _carData.tripStartTime = (_timeProvider) ? _timeProvider() : 0;
        Serial.println("[Trip] Manually started");
    } else {
        // Ручной стоп
        finalizeTripStats(true);
        _carData.tripState = false;
        _carData.tripStateDirty = true;
        unsigned long nowUnix = (_timeProvider) ? _timeProvider() : 0;
        _carData.lastManualStopTime = (nowUnix > 0) ? nowUnix : 1;
        Serial.println("[Trip] Manually stopped");
    }
    if (!obdEngine.isEmulationMode()) {
        storageManager.requestStateSave();
    }
}

void TripManager::integrateFuelAndCost() {
    // Расход в миллилитрах в секунду (уже с калибровкой из FuelManager)
    // Мы получаем lphValue, который не калиброван, но калибровка применяется в FuelManager.
    // Здесь используем некорректированный lph для статистики поездки — калибровка учтена в уменьшении fuelValueML.
    float lph = _carData.lphValue;
    float litersPerSecond = lph / 3600.0f;

    if (_engineState == ENGINE_RUNNING) {
        // Пробег и одометр
        float distanceKm = _carData.speedValue / 3600.0f;
        static float odoFraction = 0.0f;
        odoFraction += distanceKm;
        if (odoFraction >= 1.0f) {
            uint32_t add = (uint32_t)odoFraction;
            _carData.odoKm += add;
            odoFraction -= add;
            _carData.odoDirty = true;
        }

        if (_carData.tripState) {
            // Счётчики активной поездки
            _carData.tripValue++;
            _carData.tripTimeDirty = true;

            // Стоимость поездки (руб.)
            float fuelUsedLiters = litersPerSecond;
            _carData.tripMValue += fuelUsedLiters * _carData.fuelPrice;
            _carData.tripCostDirty = true;

            _fuelUsedThisTripL += fuelUsedLiters;
            _carData.tripFuelUsed = _fuelUsedThisTripL;
            _carData.tripFuelDirty = true;

            _distanceThisTripKm += distanceKm;
            _carData.tripDistanceKm = _distanceThisTripKm;
            _carData.tripDistDirty = true;

            // Максимальные значения за поездку
            if (_carData.speedValue > _carData.tripMaxSpeed) {
                _carData.tripMaxSpeed = _carData.speedValue;
            }
            uint16_t lph100 = (uint16_t)(_carData.lphValue * 100.0f);
            if (lph100 > _carData.tripMaxLPH) {
                _carData.tripMaxLPH = lph100;
            }
            _carData.tripEngineSeconds++;

            if (_carData.tempValue < 70) {
                _carData.tripWarmupSeconds++;
            }
            if (_carData.speedValue > 0) {
                _carData.tripDriveTime++;
            }
        }

        // Дневная статистика
        if (_carData.speedValue > _carData.dayAccMaxSpeed) {
            _carData.dayAccMaxSpeed = _carData.speedValue;
        }
        uint16_t lph100 = (uint16_t)(_carData.lphValue * 100.0f);
        if (lph100 > _carData.dayAccMaxLPH) {
            _carData.dayAccMaxLPH = lph100;
        }
        _carData.dayAccEngineSeconds++;
        _carData.dayAccDistance += (uint32_t)(_carData.speedValue * 1000.0f / 3600.0f);
        _carData.dayAccFuelUsed += litersPerSecond;
        if (_carData.tempValue < 70) {
            _carData.dayAccWarmupSeconds++;
        }
        if (_carData.speedValue > 0) {
            _carData.dayAccDriveTime++;
        }
    } else {
        // Двигатель заглушен — пауза в поездке
        if (_carData.tripState) {
            _carData.tripValue++;          // время паузы тоже считается?
            _carData.tripTimeDirty = true;
            _carData.tripPauseValue++;
            _carData.tripPauseDirty = true;
        }
    }
    updateRange();
}

void TripManager::updateRange() {
    float lkm = 10.0f;  // литров на 100 км по умолчанию
    if (_driveActive && _driveDistance > 100) {
        if (_driveFuelUsedL > 0.001f) {
            lkm = (_driveFuelUsedL / (_driveDistance / 1000.0f)) * 100.0f;
        }
    } else {
        if (_carData.lphValue > 0.1f && _carData.speedValue > 5) {
            lkm = _carData.lphValue / (_carData.speedValue / 100.0f);
        }
    }
    if (lkm < 2) lkm = 2;
    if (lkm > 30) lkm = 30;
    float range = (_carData.fuelValueML / 1000.0f) / lkm * 100.0f;
    if (range > _carData.rangeMax) range = _carData.rangeMax;
    if (range < 0) range = 0;
    if (abs(range - _lastRangeValue) > 1) {
        _lastRangeValue = (int)range;
        _carData.rangeValue = (int)range;
        _carData.rangeDirty = true;
    }
}

void TripManager::checkAutoStop() {
    // Автоматическое завершение поездки при длительном простое
    if (_engineState == ENGINE_OFF && _carData.tripState) {
        uint32_t offTime = (millis() - _engineOffSince) / 60000; // минуты
        // Для автоматической поездки (не ручной) используем короткий таймаут
        // Для ручной поездки — не завершаем (убрано по нашему решению)
        // Здесь мы не знаем, ручная поездка или автоматическая? Ручная отличается тем, что была вызвана toggleTrip() и не имеет таймера автостопа.
        // Реализуем: если поездка не была начата вручную (т.е. стартовала автоматически), то завершаем по короткому таймауту.
        // Признак ручной поездки: tripStatIsManual? Он устанавливается при финализации, а не при старте. Поэтому используем другой флаг — у нас его нет.
        // Пока оставим общую логику: если время простоя превышает tripAutoStopTimeout (60 мин), завершаем.
        if (offTime >= _carData.tripAutoStopTimeout) {
            finalizeTripStats(false);
            _carData.tripState = false;
            _carData.tripStateDirty = true;
            Serial.println("[Trip] Auto-stopped (timeout)");
            if (!obdEngine.isEmulationMode()) {
                storageManager.requestStateSave();
            }
        }
    }
}

void TripManager::finalizeTripStats(bool isManual) {
    if (_carData.tripStatPending) {
        Serial.println("[Trip] Stats pending, skipping overwrite");
        return;
    }
    _carData.tripStatStartTime = _carData.tripStartTime;
    _carData.tripStatIsManual = isManual;
    _carData.tripStatDuration = _carData.tripValue;
    _carData.tripStatPauseTime = _carData.tripPauseValue;
    _carData.tripStatPauseCount = _carData.tripPauseCounter;
    _carData.tripStatDistance = (uint32_t)(_carData.tripDistanceKm * 1000.0f);
    _carData.tripStatFuelUsed = _carData.tripFuelUsed;
    _carData.tripStatMaxSpeed = _carData.tripMaxSpeed;
    _carData.tripStatMaxLPH = _carData.tripMaxLPH;
    _carData.tripStatAvgThrottleRel = (_carData.tripEngineSeconds > 0)
                                      ? (_carData.tripAvgThrottleRelAccum / _carData.tripEngineSeconds) : 0;
    _carData.tripStatMaxThrottleRel = _carData.tripMaxThrottleRel;
    _carData.tripStatWarmupSeconds = _carData.tripWarmupSeconds;
    _carData.tripStatAggressiveCount = _carData.tripAggressiveCount;
    _carData.tripStatFullThrottleCount = _carData.tripFullThrottleCount;
    _carData.tripStatDriveTime = _carData.tripDriveTime;

    _carData.dayTripCounter++;
    if (!obdEngine.isEmulationMode()) {
        storageManager.requestStateSave();
    }
}

void TripManager::resetTripCounters() {
    _carData.tripValue = 0;
    _carData.tripPauseValue = 0;
    _carData.tripMValue = 0.0f;
    _carData.tripDistanceKm = 0.0f;
    _carData.tripFuelUsed = 0.0f;
    _carData.tripMaxSpeed = 0;
    _carData.tripMaxLPH = 0;
    _carData.tripAvgThrottleRelAccum = 0;
    _carData.tripMaxThrottleRel = 0;
    _carData.tripPauseCounter = 0;
    _carData.tripEngineSeconds = 0;
    _carData.tripWarmupSeconds = 0;
    _carData.tripAggressiveCount = 0;
    _carData.tripFullThrottleCount = 0;
    _carData.tripDriveTime = 0;

    _carData.tripTimeDirty = true;
    _carData.tripPauseDirty = true;
    _carData.tripCostDirty = true;
    _carData.tripDistDirty = true;
    _carData.tripFuelDirty = true;
}

void TripManager::updateDayStats(uint32_t nowUnix) {
    if (nowUnix == 0) return;  // время не синхронизировано
    uint32_t dayStart = nowUnix - (nowUnix % 86400);
    if (dayStart != _carData.dayAccDate) {
        if (_carData.dayAccDate != 0 && !_carData.dayStatPending) {
            _carData.dayStatDate = _carData.dayAccDate;
            _carData.dayStatValid = true;
            _carData.dayStatEngineSeconds = _carData.dayAccEngineSeconds;
            _carData.dayStatDistance = _carData.dayAccDistance;
            _carData.dayStatFuelUsed = _carData.dayAccFuelUsed;
            _carData.dayStatMaxSpeed = _carData.dayAccMaxSpeed;
            _carData.dayStatMaxLPH = _carData.dayAccMaxLPH;
            _carData.dayStatAvgThrottleRel = (_carData.dayAccEngineSeconds > 0)
                ? (_carData.dayAccAvgThrottleRelAccum / _carData.dayAccEngineSeconds) : 0;
            _carData.dayStatMaxThrottleRel = _carData.dayAccMaxThrottleRel;
            _carData.dayStatWarmupSeconds = _carData.dayAccWarmupSeconds;
            _carData.dayStatAggressiveCount = _carData.dayAccAggressiveCount;
            _carData.dayStatFullThrottleCount = _carData.dayAccFullThrottleCount;
            _carData.dayStatDriveTime = _carData.dayAccDriveTime;
            _carData.dayStatFirstStart = _carData.dayFirstStart;
            _carData.dayStatLastStop = (_carData.dayLastStop > 0) ? _carData.dayLastStop : nowUnix;
            _carData.dayStatTripCount = _carData.dayTripCounter;
            _carData.dayStatDriveCycleCount = _carData.driveCycleCount;
        }
        // Сброс дневных аккумуляторов
        _carData.dayAccEngineSeconds = 0;
        _carData.dayAccDistance = 0;
        _carData.dayAccFuelUsed = 0.0f;
        _carData.dayAccMaxSpeed = 0;
        _carData.dayAccMaxLPH = 0;
        _carData.dayAccAvgThrottleRelAccum = 0;
        _carData.dayAccMaxThrottleRel = 0;
        _carData.dayAccWarmupSeconds = 0;
        _carData.dayAccAggressiveCount = 0;
        _carData.dayAccFullThrottleCount = 0;
        _carData.dayAccDriveTime = 0;
        _carData.dayAccDate = dayStart;
        _carData.dayFirstStart = 0;
        _carData.dayLastStop = 0;
        _carData.dayTripCounter = 0;
    }
}

void TripManager::startDriveIfNeeded() {
    // Заезд стартует только при начале движения (скорость > 0)
    if (!_driveActive && _engineState == ENGINE_RUNNING && _carData.speedValue > 0) {
        _driveActive = true;
        _driveStartTime = _carData.systemTime;
        _driveEndTime = 0;
        _driveDuration = 0;
        _driveDistance = 0;
        _driveFuelUsedL = 0.0f;
        _driveMaxSpeed = 0;
        _driveMaxLPH = 0;
        _driveAvgThrottleAccum = 0;
        _driveThrottleSamples = 0;
        _driveMaxThrottleRel = 0;
        _driveWarmupSeconds = 0;
        _driveAggressiveCount = 0;
        _driveFullThrottleCount = 0;
        _driveMoved = false;
        _driveMoveSeconds = 0;
        _drivePrevThrottleRel = _carData.throttleRelPos;
    }
}

void TripManager::updateDrive() {
    if (!_driveActive) return;
    if (_engineState == ENGINE_RUNNING) {
        if (_carData.speedValue > 0) {
            _driveDuration++;
            _driveMoveSeconds++;
            if (_driveMoveSeconds > 60) _driveMoved = true;
        }
        float distM = _carData.speedValue * 1000.0f / 3600.0f;
        _driveDistance += (uint32_t)distM;
        float litersPerSec = _carData.lphValue / 3600.0f;
        _driveFuelUsedL += litersPerSec;

        if (_carData.speedValue > _driveMaxSpeed) _driveMaxSpeed = _carData.speedValue;
        uint16_t lph100 = (uint16_t)(_carData.lphValue * 100.0f);
        if (lph100 > _driveMaxLPH) _driveMaxLPH = lph100;

        uint16_t throttleRel = (uint16_t)(_carData.throttleRelPos * 100.0f);
        _driveAvgThrottleAccum += throttleRel;
        _driveThrottleSamples++;
        if (throttleRel > _driveMaxThrottleRel) _driveMaxThrottleRel = throttleRel;

        float prev = _drivePrevThrottleRel;
        float curr = _carData.throttleRelPos;
        if (curr - prev > 50.0f && curr > 80.0f) {
            _driveAggressiveCount++;
            _carData.tripAggressiveCount++;
            _carData.dayAccAggressiveCount++;
        }
        _drivePrevThrottleRel = curr;

        if (_carData.throttleRelPos > 98.0f) {
            _driveFullThrottleCount++;
            _carData.tripFullThrottleCount++;
            _carData.dayAccFullThrottleCount++;
        }

        if (_carData.tempValue < 70) {
            _driveWarmupSeconds++;
        }

        _carData.tripAvgThrottleRelAccum += throttleRel;
        if (throttleRel > _carData.tripMaxThrottleRel) _carData.tripMaxThrottleRel = throttleRel;
        _carData.dayAccAvgThrottleRelAccum += throttleRel;
        if (throttleRel > _carData.dayAccMaxThrottleRel) _carData.dayAccMaxThrottleRel = throttleRel;
    } else {
        // Двигатель заглушен — ожидаем 10 минут, затем завершаем заезд
        if (_driveEndTime == 0) {
            _driveEndTime = millis();
        } else if (millis() - _driveEndTime > 600000) {
            if (_driveMoved) finalizeDrive();
            _driveActive = false;
        }
    }
}

void TripManager::finalizeDrive() {
    if (_carData.driveCycleCount >= 15) {
        for (int i = 0; i < 14; i++) {
            _carData.driveCycles[i] = _carData.driveCycles[i+1];
        }
        _carData.driveCycleCount = 14;
    }
    DriveCycle* dc = &_carData.driveCycles[_carData.driveCycleCount];
    dc->startTime = _driveStartTime;
    dc->endTime = _carData.systemTime;
    dc->duration = _driveDuration;
    dc->distance = _driveDistance;
    dc->fuelUsed = _driveFuelUsedL;
    dc->maxSpeed = _driveMaxSpeed;
    dc->maxLPH = _driveMaxLPH;
    dc->avgThrottleRel = (_driveThrottleSamples > 0) ? (_driveAvgThrottleAccum / _driveThrottleSamples) : 0;
    dc->maxThrottleRel = _driveMaxThrottleRel;
    dc->aggressiveCount = _driveAggressiveCount;
    dc->fullThrottleCount = _driveFullThrottleCount;
    dc->warmupSeconds = _driveWarmupSeconds;
    _carData.driveCycleCount++;
}

void TripManager::clearDriveCycles() {
    _carData.driveCycleCount = 0;
    memset(_carData.driveCycles, 0, sizeof(_carData.driveCycles));
}