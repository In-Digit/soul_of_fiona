#ifndef FUEL_MANAGER_H
#define FUEL_MANAGER_H

#include <Arduino.h>
#include "CarData.h"

class FuelManager {
public:
    FuelManager(CarData& data);
    void begin();
    void process();      // вызывается раз в секунду

    // Обработка заправки (вызывается из GUI)
    void handleRefuel(float liters, bool isFullTank, uint32_t odoMiles, float fuelPrice);

private:
    CarData& _carData;
    uint32_t _lastProcessTime;
    float _lastSavedFuel;       // последнее СОХРАНЁННОЕ значение fuelValueML, переведённое в литры для контроля
    uint32_t _pendingFuelML;    // значение, для которого запрошено сохранение (в мл)
    bool _savePending;          // флаг наличия незавершённого запроса на сохранение

    // Внутренняя функция для расчёта и применения нового коэффициента калибровки
    void updateCalibration(float calculatedUsed, float realUsed);
};

#endif