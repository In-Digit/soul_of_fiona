#include "FuelManager.h"
#include "StorageManager.h"
#include "OBDEngine.h"

extern StorageManager storageManager;
extern OBDEngine obdEngine;

// Параметры калибровки
#define K_MIN 0.7f
#define K_MAX 1.3f
#define LEARNING_RATE 0.2f   // вес нового значения (0..1)
#define DEAD_ZONE 0.03f      // 3% зона нечувствительности
#define USEFUL_TANK_ML 50000 // полезный объём бака в мл (50 л)

FuelManager::FuelManager(CarData& data)
    : _carData(data), _lastProcessTime(0), _lastSavedFuel(0),
      _pendingFuelML(0), _savePending(false)
{
    // _lastSavedFuel будет установлено в begin() по текущему fuelValueML
}

void FuelManager::begin() {
    _lastProcessTime = millis();
    // Переводим текущий остаток из миллилитров в литры для контроля сохранения
    _lastSavedFuel = _carData.fuelValueML / 1000.0f;
    _pendingFuelML = _carData.fuelValueML;
    _savePending = false;

    if (_carData.initialOdoKm > 0 && _carData.initialFuel > 0) {
        // Восстанавливаем одометр и уровень после ручного ввода
        _carData.odoKm = _carData.initialOdoKm;
        // initialFuel задана в литрах, переводим в мл
        _carData.fuelValueML = (uint32_t)(_carData.initialFuel * 1000.0f);
        _carData.odoDirty = true;
        _carData.fuelDirty = true;
        _carData.initialOdoKm = 0;
        _carData.initialFuel = 0;
        Serial.printf("[Fuel] Initial odo: %u km, fuel: %.1f L (%u ml)\n",
                      _carData.odoKm, _carData.fuelValueML / 1000.0f, _carData.fuelValueML);
    }
}

void FuelManager::process() {
    uint32_t now = millis();
    if (now - _lastProcessTime < 1000) return;
    _lastProcessTime = now;

    // Интегрируем расход только при работающем двигателе
    if (_carData.rpmValue > 100) {
        // lphValue (л/ч) -> миллилитры в секунду
        float litersPerSecond = _carData.lphValue / 3600.0f;
        // применяем калибровочный коэффициент
        float mlPerSecond = litersPerSecond * _carData.fuelCalibrationFactor * 1000.0f;
        // вычитаем из остатка (не ниже нуля)
        if (_carData.fuelValueML >= (uint32_t)mlPerSecond) {
            _carData.fuelValueML -= (uint32_t)mlPerSecond;
        } else {
            _carData.fuelValueML = 0;
            _carData.calibrationNeeded = true;  // достигли учётного нуля
        }
        _carData.fuelDirty = true;

        // Интеграция для калибровки: считаем расход в литрах
        _carData.calculatedFuelSinceLastFull += litersPerSecond;
    }

    // Запрос на сохранение, если уровень изменился более чем на 100 мл
    float currentLiters = _carData.fuelValueML / 1000.0f;
    if (fabs(currentLiters - _lastSavedFuel) >= 0.1f && !_savePending) {
        _pendingFuelML = _carData.fuelValueML;
        _savePending = true;
        if (!obdEngine.isEmulationMode()) {
            storageManager.requestStateSave();
        }
    }

    // Если запрос сохранения обработан StorageManager'ом, фиксируем новое значение
    if (_savePending && !storageManager.isStateSaveRequested()) {
        _lastSavedFuel = _pendingFuelML / 1000.0f;
        _savePending = false;
    }
}

void FuelManager::handleRefuel(float liters, bool isFullTank, uint32_t odoMiles, float fuelPrice) {
    uint32_t odoKm = (uint32_t)(odoMiles * 1.60934f);

    // Если полный бак (залито до горловины)
    if (isFullTank) {
        // Фиксируем одометр (защищён мьютексом снаружи)
        _carData.odoKm = odoKm;
        _carData.odoDirty = true;

        if (!_carData.hasFirstFullTank) {
            // Самая первая полная заправка — только запоминаем точку отсчёта
            _carData.lastFullOdoKm = odoKm;
            _carData.totalRefuelSinceLastFull = 0;
            _carData.calculatedFuelSinceLastFull = 0;
            _carData.fuelValueML = USEFUL_TANK_ML;   // 50 л
            _carData.hasFirstFullTank = true;
            _carData.calibrationNeeded = false;
        } else {
            // Последующие полные заправки: считаем реальный расход и обновляем коэффициент
            float realFuelUsed = _carData.totalRefuelSinceLastFull + liters;
            float calculatedUsed = _carData.calculatedFuelSinceLastFull;
            if (realFuelUsed > 0 && calculatedUsed > 0) {
                updateCalibration(calculatedUsed, realFuelUsed);
            }
            // Сбрасываем накопительные счётчики
            _carData.lastFullOdoKm = odoKm;
            _carData.totalRefuelSinceLastFull = 0;
            _carData.calculatedFuelSinceLastFull = 0;
            _carData.fuelValueML = USEFUL_TANK_ML;
            _carData.calibrationNeeded = false;
        }

        // Обновляем цену
        _carData.fuelPrice = fuelPrice;
        _carData.fuelDirty = true;
    } else {
        // Частичная заправка (долив): добавляем литры к остатку
        uint32_t addML = (uint32_t)(liters * 1000.0f);
        if (_carData.fuelValueML + addML > USEFUL_TANK_ML) {
            // Если ввели больше, чем влезает до полезного максимума, обрезаем и сигнализируем
            _carData.fuelValueML = USEFUL_TANK_ML;
            _carData.calibrationNeeded = true;
        } else {
            _carData.fuelValueML += addML;
        }
        _carData.totalRefuelSinceLastFull += liters;
        _carData.fuelPrice = fuelPrice;
        _carData.fuelDirty = true;
    }

    Serial.printf("[Fuel] Refuel: %.1f L, total: %.1f L (calib factor: %.3f)\n", 
                  liters, _carData.fuelValueML / 1000.0f, _carData.fuelCalibrationFactor);

    // Сбрасываем флаг сохранения и сохраняем состояние
    _lastSavedFuel = _carData.fuelValueML / 1000.0f;
    _savePending = false;
    if (!obdEngine.isEmulationMode()) {
        storageManager.requestStateSave();
    }
}

// Экспоненциальное сглаживание с зоной нечувствительности
void FuelManager::updateCalibration(float calculatedUsed, float realUsed) {
    if (realUsed <= 0.0f) return;
    float K_raw = calculatedUsed / realUsed;
    // Если отклонение меньше 3%, игнорируем
    if (fabs(K_raw - _carData.fuelCalibrationFactor) < DEAD_ZONE * _carData.fuelCalibrationFactor) {
        Serial.printf("[Fuel] Calibration skipped (within dead zone): raw %.3f, old %.3f\n", K_raw, _carData.fuelCalibrationFactor);
        return;
    }
    float K_new = _carData.fuelCalibrationFactor + LEARNING_RATE * (K_raw - _carData.fuelCalibrationFactor);
    // Ограничиваем разумным диапазоном
    if (K_new < K_MIN) K_new = K_MIN;
    if (K_new > K_MAX) K_new = K_MAX;
    Serial.printf("[Fuel] Calibration update: raw %.3f -> new %.3f (old %.3f)\n", K_raw, K_new, _carData.fuelCalibrationFactor);
    _carData.fuelCalibrationFactor = K_new;
}