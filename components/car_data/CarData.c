#include "CarData.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"

// Глобальный экземпляр CarData, размещённый в PSRAM
static CarData *g_carData = NULL;
static SemaphoreHandle_t g_carDataMutex = NULL;

void CarData_init(CarData* data) {
    if (data == NULL) return;
    memset(data, 0, sizeof(CarData));

    data->colorCyan   = COLOR_CYAN;
    data->colorOrange = COLOR_ORANGE;
    data->colorRed    = COLOR_RED;
    data->colorGreen  = COLOR_GREEN;
    data->colorBlue   = COLOR_BLUE;
    data->colorYellow = COLOR_YELLOW;

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

    data->fuelValue   = 50.0f;
    data->odoKm       = 200000;
    data->tripState   = false;
    data->lphValue    = 0.0f;
    data->rangeValue  = 0;
    data->tripValue   = 0;
    data->tripPauseValue = 0;
    data->tripDistanceKm = 0.0f;
    data->tripMValue  = 0.0f;
    data->tripFuelUsed = 0.0f;

    data->fuelPrice = 63.7f;
    data->fuelCalibrationFactor = 1.0f;
    data->lastFullOdoKm = 0;
    data->totalRefuelSinceLastFull = 0.0f;
    data->calculatedFuelSinceLastFull = 0.0f;
    data->hasFirstFullTank = false;
    data->initialOdoKm = 0;
    data->initialFuel = 0.0f;
    data->calibrationNeeded = false;

    data->tripAutoStopTimeout = 60;
    data->lastManualStopTime = 0;
    data->sleepTimeout = 15;

    data->ntcHeaterOut = 0.0f;
    data->ntcCabin     = 0.0f;
    data->ntcDriverFeet = 0.0f;
    data->ntcPassengerFeet = 0.0f;
    data->ntcTrunk = 0.0f;
    data->ntcCabinCenter = 0.0f;

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

    strncpy(data->wifiSsid1, "Tri-Al(m)", sizeof(data->wifiSsid1)-1);
    strncpy(data->wifiPass1, "Ford-Fiona", sizeof(data->wifiPass1)-1);
    strncpy(data->wifiSsid2, "Tri-AL", sizeof(data->wifiSsid2)-1);
    strncpy(data->wifiPass2, "Aq1Sw2De3Fr4", sizeof(data->wifiPass2)-1);
    strncpy(data->wifiSsid3, "KbKb", sizeof(data->wifiSsid3)-1);
    strncpy(data->wifiPass3, "1234567890", sizeof(data->wifiPass3)-1);
    strncpy(data->apSsid, "Fiona-WiFi", sizeof(data->apSsid)-1);
    strncpy(data->apPass, "Ford-Fiona", sizeof(data->apPass)-1);

    strncpy(data->btName, "Fiona-BT", sizeof(data->btName)-1);
    strncpy(data->btMac,  "00:1D:A5:07:05:17", sizeof(data->btMac)-1);
    strncpy(data->btPin,  "1234", sizeof(data->btPin)-1);

    data->fanSetpoint = 95.5f;
    data->fanKp = 30.0f;
    data->fanKi = 2.0f;
    data->fanKd = 5.0f;

    data->climateSetpoint = 22.0f;

    data->obdConnected = false;
    data->wifiConnected = false;
    data->wifiRSSI = 0;
    data->wifiRSSIDirty = false;
    data->uartArduinoAlive = false;
    data->uartEsp32Alive = false;

    data->internetAvailable = false;
    data->internetDirty = false;

    data->apiRequestPending = false;
    data->apiRequestId = 0;
    data->apiResponseValid = false;

    data->activePresetId = 0;
    data->presetPending = false;

    data->systemTime = 0;
    data->lastObdUpdate = 0;
    data->lastPresetUpdate = 0;
    data->lastApiResponse = 0;

    data->configDirty = false;
    data->requestConfigSync = false;

    data->screensaver_timeout_sec = 300;
    data->time_synced = false;
    data->time_received_this_boot = false;

    memset(&data->lastTripStats, 0, sizeof(TripStats));
    memset(&data->lastDayStats, 0, sizeof(DayStats));
    memset(&data->lastRefuel, 0, sizeof(RefuelRecord));

    data->trip_stats_pending = false;
    data->day_stats_pending = false;
    data->day_requested_today = false;

    memset(&data->trip_rx, 0, sizeof(data->trip_rx));
    memset(&data->day_rx, 0, sizeof(data->day_rx));

    // --------------- ЯРКОСТЬ ---------------
    data->backlight_brightness     = 80;
    data->backlight_min_brightness = 5;
    data->light_threshold_dark     = 20;
    data->light_threshold_bright   = 80;
    data->ambient_light_pct        = 100;
    data->ambient_light_synth      = 50;
    data->ambient_light_raw        = 0;

    // --------------- ТЕМА ФОНА ---------------
    data->bg_theme = 0;

    // --------------- РУЧНАЯ УСТАНОВКА ВРЕМЕНИ ---------------
    data->time_set_manually = false;

    // --------------- ТЕПЛОВАЯ ПАМЯТЬ ---------------
    data->last_valid_coolant_temp = INT_MIN;

    // --------------- ЦВЕТА ТОНАЛЬНОСТЕЙ ---------------
    data->color_tone_neutral = 0x9EEFFC;
    data->color_tone_funny   = 0xFFB6C1;
    data->color_tone_serious = 0x00FF00;
}

uint32_t CarData_getFuelColor(const CarData* data) {
    if (data->fuelValue < data->fuelRedThreshold)      return data->colorRed;
    if (data->fuelValue < data->fuelYellowThreshold)   return data->colorYellow;
    return data->colorGreen;   // было colorCyan
}

uint32_t CarData_getBatteryColor(const CarData* data) {
    if (data->batValue < data->battLowThreshold || data->batValue > data->battHighThreshold)
        return data->colorRed;
    return data->colorGreen;   // было colorCyan
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

bool CarData_Init(void) {
    if (g_carData != NULL) {
        return true;
    }

    g_carData = (CarData*) heap_caps_calloc(1, sizeof(CarData), MALLOC_CAP_SPIRAM);
    if (g_carData == NULL) {
        return false;
    }

    g_carDataMutex = xSemaphoreCreateMutex();
    if (g_carDataMutex == NULL) {
        heap_caps_free(g_carData);
        g_carData = NULL;
        return false;
    }

    CarData_init(g_carData);
    return true;
}

CarData* CarData_Get(void) {
    return g_carData;
}

bool CarData_Lock(uint32_t timeout_ms) {
    if (g_carDataMutex == NULL) return false;
    return xSemaphoreTake(g_carDataMutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void CarData_Unlock(void) {
    if (g_carDataMutex != NULL) {
        xSemaphoreGive(g_carDataMutex);
    }
}

void CarData_set_time_synced(bool synced) {
    CarData *data = CarData_Get();
    if (data) {
        CarData_Lock(10);
        data->time_synced = synced;
        CarData_Unlock();
    }
}