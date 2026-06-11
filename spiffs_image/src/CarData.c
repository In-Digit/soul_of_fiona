#include "CarData.h"
#include <string.h>
#include <stdio.h>

// Мьютекс для глобальной структуры CarData
static SemaphoreHandle_t g_carDataMutex = NULL;

/**
 * @brief Инициализация мьютекса (вызывается один раз при старте).
 */
static void CarData_initMutex() {
    if (g_carDataMutex == NULL) {
        g_carDataMutex = xSemaphoreCreateMutex();
    }
}

bool CarData_Lock(uint32_t timeout_ms) {
    if (g_carDataMutex == NULL) CarData_initMutex();
    return xSemaphoreTake(g_carDataMutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void CarData_Unlock(void) {
    if (g_carDataMutex != NULL) {
        xSemaphoreGive(g_carDataMutex);
    }
}

void CarData_init(CarData* data) {
    if (data == NULL) return;
    memset(data, 0, sizeof(CarData));

    // Цвета по умолчанию
    data->colorCyan   = COLOR_CYAN;
    data->colorOrange = COLOR_ORANGE;
    data->colorRed    = COLOR_RED;
    data->colorGreen  = COLOR_GREEN;
    data->colorBlue   = COLOR_BLUE;
    data->colorYellow = COLOR_YELLOW;

    // Пороги шкал
    data->fuelRedThreshold   = FUEL_RED_THRESHOLD;
    data->fuelYellowThreshold = FUEL_YELLOW_THRESHOLD;
    data->battLowThreshold   = BATT_LOW_THRESHOLD;
    data->battHighThreshold  = BATT_HIGH_THRESHOLD;
    data->tempCold           = TEMP_COLD;
    data->tempNormal         = TEMP_NORMAL;
    data->tempWarm           = TEMP_WARM;
    data->tempHot            = TEMP_HOT;
    data->speedLow           = SPEED_LOW;
    data->speedMidLow        = SPEED_MID_LOW;
    data->speedMidHigh       = SPEED_MID_HIGH;
    data->speedHighLow       = SPEED_HIGH_LOW;
    data->speedHighHigh      = SPEED_HIGH_HIGH;
    data->speedVHighLow      = SPEED_VHIGH_LOW;
    data->speedVHighHigh     = SPEED_VHIGH_HIGH;
    data->rpmBlueMax         = RPM_BLUE_MAX;
    data->rpmCyanMax         = RPM_CYAN_MAX;
    data->rpmYellowMax       = RPM_YELLOW_MAX;
    data->lphBlueMax         = LPH_BLUE_MAX;
    data->lphCyanMax         = LPH_CYAN_MAX;
    data->lphYellowMax       = LPH_YELLOW_MAX;

    // Диапазоны шкал
    data->fuelMin   = FUEL_MIN;
    data->fuelMax   = FUEL_MAX;
    data->fuelStep  = FUEL_STEP;
    data->rangeMin  = RANGE_MIN;
    data->rangeMax  = RANGE_MAX;
    data->rangeStep = RANGE_STEP;
    data->battMin   = BATT_MIN;
    data->battMax   = BATT_MAX;
    data->battStep  = BATT_STEP;
    data->tempMin   = TEMP_MIN;
    data->tempMax   = TEMP_MAX;
    data->tempStep  = TEMP_STEP;
    data->speedMin  = SPEED_MIN;
    data->speedMax  = SPEED_MAX;
    data->speedStep = SPEED_STEP;
    data->rpmMin    = RPM_MIN;
    data->rpmMax    = RPM_MAX;
    data->rpmStep   = RPM_STEP;
    data->tripMMin  = TRIPM_MIN;
    data->tripMMax  = TRIPM_MAX;
    data->tripMStep = TRIPM_STEP;
    data->lphMin    = LPH_MIN;
    data->lphMax    = LPH_MAX;
    data->lphStep   = LPH_STEP;

    // Начальное состояние топлива: 50 л = 50000 мл
    data->fuelValueML = 50000;
    data->odoKm       = 200000;
    data->tripState   = false;
    data->lphValue    = 0.0f;
    data->rangeValue  = 0;
    data->tripValue   = 0;
    data->tripPauseValue = 0;
    data->tripDistanceKm = 0.0f;
    data->tripMValue  = 0.0f;
    data->tripFuelUsed = 0.0f;

    // Калибровка топлива
    data->fuelPrice = 63.7f;
    data->fuelCalibrationFactor = 1.0f;
    data->lastFullOdoKm = 0;
    data->totalRefuelSinceLastFull = 0.0f;
    data->calculatedFuelSinceLastFull = 0.0f;
    data->hasFirstFullTank = false;
    data->initialOdoKm = 0;
    data->initialFuel = 0.0f;
    data->calibrationNeeded = false;

    // Параметры поездок
    data->tripAutoStopTimeout = 60;
    data->lastManualStopTime = 0;
    data->sleepTimeout = 15;

    // Датчики NTC
    data->ntcHeaterOut = 0.0f;
    data->ntcCabin     = 0.0f;
    data->ntcDriverFeet = 0.0f;
    data->ntcPassengerFeet = 0.0f;
    data->ntcTrunk = 0.0f;
    data->ntcCabinCenter = 0.0f;

    // Исполнители
    data->fanCurrentPWM1 = 0;
    data->fanCurrentPWM2 = 0;
    data->climateCurrentPWM = 0;
    data->trunkFanPWM = 0;
    data->damperPosition = 0;
    data->airDirection = 0;

    data->fanAutoMode = true;
    data->climateAutoMode = true;
    data->damperAutoMode = true;
    data->fanManualPWM = 0;
    data->climateManualPWM = 0;

    // Новые поля климата
    data->fanControlEnabled = false;
    data->arduinoPresent = false;
    data->fanMode = 0;
    data->fan1CalibStartPoint = 0;
    data->fan1CalibStopPoint = 0;
    data->fan1NoiseLow = 0;
    data->fan1NoiseHigh = 0;
    data->fan2CalibStartPoint = 0;
    data->fan2CalibStopPoint = 0;
    data->fan2NoiseLow = 0;
    data->fan2NoiseHigh = 0;
    data->climatePreset = 1;
    data->climateCalibNoiseLow = 0;
    data->climateCalibNoiseHigh = 0;
    data->climateCalibStartPoint = 0;
    data->climateCalibStopPoint = 0;
    data->coolantTempOffset = 0.0f;

    // WiFi
    strncpy(data->wifiSsid1, "Tri-Al(m)", sizeof(data->wifiSsid1)-1);
    strncpy(data->wifiPass1, "Ford-Fiona", sizeof(data->wifiPass1)-1);
    strncpy(data->wifiSsid2, "Tri-AL", sizeof(data->wifiSsid2)-1);
    strncpy(data->wifiPass2, "Aq1Sw2De3Fr4", sizeof(data->wifiPass2)-1);
    strncpy(data->wifiSsid3, "KbKb", sizeof(data->wifiSsid3)-1);
    strncpy(data->wifiPass3, "1234567890", sizeof(data->wifiPass3)-1);
    strncpy(data->apSsid, "Fiona-WiFi", sizeof(data->apSsid)-1);
    strncpy(data->apPass, "Ford-Fiona", sizeof(data->apPass)-1);

    // Bluetooth
    strncpy(data->btName, "Fiona-BT", sizeof(data->btName)-1);
    strncpy(data->btMac,  "00:1D:A5:07:05:17", sizeof(data->btMac)-1);
    strncpy(data->btPin,  "1234", sizeof(data->btPin)-1);

    // PID вентиляторов
    data->fanSetpoint = 95.5f;
    data->fanKp = 30.0f;
    data->fanKi = 2.0f;
    data->fanKd = 5.0f;

    data->climateSetpoint = 22.0f;

    // Статусы связи
    data->obdConnected = false;
    data->wifiConnected = false;
    data->uartArduinoAlive = false;
    data->uartEsp32Alive = false;
    data->wifiRSSI = 0;
    data->wifiRSSIDirty = false;

    data->internetAvailable = false;
    data->internetDirty = false;

    // API
    data->apiRequestPending = false;
    data->apiRequestId = 0;
    data->apiResponseValid = false;

    // Пресеты
    data->activePresetId = 0;
    data->presetPending = false;

    // Временные метки
    data->systemTime = 0;
    data->systemSyncTime = 0;
    data->lastObdUpdate = 0;
    data->lastPresetUpdate = 0;
    data->lastApiResponse = 0;

    data->configDirty = false;
    data->requestConfigSync = false;

    // MPU-6050
    data->imuCalibrated = false;

    // Мьютекс будет создан при первом обращении
}

// Остальные функции без изменений, кроме getFuelColor, который теперь использует fuelValueML
uint32_t CarData_getFuelColor(const CarData* data) {
    // Переводим миллилитры в литры для сравнения с порогами
    float fuelLiters = data->fuelValueML / 1000.0f;
    if (fuelLiters < data->fuelRedThreshold)      return data->colorRed;
    if (fuelLiters < data->fuelYellowThreshold)   return data->colorYellow;
    return data->colorCyan;
}

uint32_t CarData_getBatteryColor(const CarData* data) {
    if (data->batValue < data->battLowThreshold || data->batValue > data->battHighThreshold)
        return data->colorRed;
    return data->colorCyan;
}

uint32_t CarData_getTempColor(const CarData* data) {
    if (data->tempValue < data->tempCold)        return data->colorBlue;
    if (data->tempValue <= data->tempNormal)     return data->colorGreen;
    if (data->tempValue <= data->tempWarm)       return data->colorCyan;
    if (data->tempValue <= data->tempHot)        return data->colorYellow;
    return data->colorRed;
}

uint32_t CarData_getSpeedColor(const CarData* data) {
    int s = data->speedValue;
    if (s >= 0 && s <= data->speedLow) return data->colorBlue;
    if ((s >= data->speedMidLow && s <= data->speedMidHigh) ||
        (s >= data->speedHighLow && s <= data->speedHighHigh))
        return data->colorOrange;
    if (s >= data->speedVHighLow && s <= data->speedVHighHigh) return data->colorYellow;
    if (s > data->speedVHighHigh) return data->colorRed;
    return data->colorCyan;
}

uint32_t CarData_getRPMColor(const CarData* data) {
    if (data->rpmValue <= data->rpmBlueMax)   return data->colorBlue;
    if (data->rpmValue <= data->rpmCyanMax)   return data->colorCyan;
    if (data->rpmValue <= data->rpmYellowMax) return data->colorYellow;
    return data->colorRed;
}

uint32_t CarData_getLPHColor(const CarData* data) {
    if (data->lphValue <= data->lphBlueMax)   return data->colorBlue;
    if (data->lphValue <= data->lphCyanMax)   return data->colorCyan;
    if (data->lphValue <= data->lphYellowMax) return data->colorYellow;
    return data->colorRed;
}

void CarData_formatTimeHMS(uint32_t totalSeconds, char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize < 9) return;
    uint16_t hours   = totalSeconds / 3600;
    uint8_t  minutes = (totalSeconds % 3600) / 60;
    uint8_t  seconds = totalSeconds % 60;
    snprintf(buffer, bufferSize, "%02u:%02u:%02u", hours, minutes, seconds);
}