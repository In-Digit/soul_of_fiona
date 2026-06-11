#include "StorageManager.h"

StorageManager::StorageManager(CarData& data)
    : _carData(data), _configSaveRequested(false), _stateSaveRequested(false), _lastStateSave(0),
      _lastConfigSaveTime(0), _lastStateSaveTime(0),
      _lastConfigSaveError(false), _lastStateSaveError(false) {}

bool StorageManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return false;
    }
    return true;
}

void StorageManager::requestConfigSave() {
    _configSaveRequested = true;
}

void StorageManager::requestStateSave() {
    _stateSaveRequested = true;
}

void StorageManager::process() {
    uint32_t now = millis();

    if (_configSaveRequested) {
        if (saveConfig()) {
            _configSaveRequested = false;
        }
    }

    if (_stateSaveRequested && (now - _lastStateSave > STATE_SAVE_DEBOUNCE)) {
        if (saveState()) {
            _stateSaveRequested = false;
            _lastStateSave = now;
        }
    }
}

bool StorageManager::loadConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("No config file, using defaults");
        return false;
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }

    deserializeConfig(doc);
    return true;
}

bool StorageManager::saveConfig() {
    StaticJsonDocument<4096> doc;
    serializeConfig(doc);

    File file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        _lastConfigSaveError = true;
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write config file");
        file.close();
        _lastConfigSaveError = true;
        return false;
    }
    file.close();
    Serial.println("Config saved");
    _lastConfigSaveTime = millis();
    _lastConfigSaveError = false;
    return true;
}

bool StorageManager::loadState() {
    File file = LittleFS.open("/state.json", "r");
    if (!file) {
        Serial.println("No state file, using defaults");
        return false;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse state file");
        return false;
    }

    deserializeState(doc);
    return true;
}

bool StorageManager::saveState() {
    StaticJsonDocument<2048> doc;
    serializeState(doc);

    File tmp = LittleFS.open("/state.tmp", "w");
    if (!tmp) {
        Serial.println("Failed to open temp state file");
        _lastStateSaveError = true;
        return false;
    }
    if (serializeJson(doc, tmp) == 0) {
        Serial.println("Failed to write temp state file");
        tmp.close();
        _lastStateSaveError = true;
        return false;
    }
    tmp.close();

    if (!LittleFS.rename("/state.tmp", "/state.json")) {
        Serial.println("Failed to rename temp state file");
        _lastStateSaveError = true;
        return false;
    }
    Serial.println("State saved");
    _lastStateSaveTime = millis();
    _lastStateSaveError = false;
    return true;
}

// -------------------------------------------------------------------
// Сериализация НАСТРОЕК (цвета, пороги, WiFi, BT, PID, калибровки)
// -------------------------------------------------------------------
void StorageManager::serializeConfig(JsonDocument& doc) {
    // Цвета
    doc["colorCyan"]   = _carData.colorCyan;
    doc["colorOrange"] = _carData.colorOrange;
    doc["colorRed"]    = _carData.colorRed;
    doc["colorGreen"]  = _carData.colorGreen;
    doc["colorBlue"]   = _carData.colorBlue;
    doc["colorYellow"] = _carData.colorYellow;

    // Пороги шкал
    doc["fuelRedThreshold"]    = _carData.fuelRedThreshold;
    doc["fuelYellowThreshold"] = _carData.fuelYellowThreshold;
    doc["battLowThreshold"]    = _carData.battLowThreshold;
    doc["battHighThreshold"]   = _carData.battHighThreshold;
    doc["tempCold"]            = _carData.tempCold;
    doc["tempNormal"]          = _carData.tempNormal;
    doc["tempWarm"]            = _carData.tempWarm;
    doc["tempHot"]             = _carData.tempHot;
    doc["speedLow"]            = _carData.speedLow;
    doc["speedMidLow"]         = _carData.speedMidLow;
    doc["speedMidHigh"]        = _carData.speedMidHigh;
    doc["speedHighLow"]        = _carData.speedHighLow;
    doc["speedHighHigh"]       = _carData.speedHighHigh;
    doc["speedVHighLow"]       = _carData.speedVHighLow;
    doc["speedVHighHigh"]      = _carData.speedVHighHigh;
    doc["rpmBlueMax"]          = _carData.rpmBlueMax;
    doc["rpmCyanMax"]          = _carData.rpmCyanMax;
    doc["rpmYellowMax"]        = _carData.rpmYellowMax;
    doc["lphBlueMax"]          = _carData.lphBlueMax;
    doc["lphCyanMax"]          = _carData.lphCyanMax;
    doc["lphYellowMax"]        = _carData.lphYellowMax;

    // Диапазоны шкал
    doc["fuelMin"]   = _carData.fuelMin;
    doc["fuelMax"]   = _carData.fuelMax;
    doc["fuelStep"]  = _carData.fuelStep;
    doc["rangeMin"]  = _carData.rangeMin;
    doc["rangeMax"]  = _carData.rangeMax;
    doc["rangeStep"] = _carData.rangeStep;
    doc["battMin"]   = _carData.battMin;
    doc["battMax"]   = _carData.battMax;
    doc["battStep"]  = _carData.battStep;
    doc["tempMin"]   = _carData.tempMin;
    doc["tempMax"]   = _carData.tempMax;
    doc["tempStep"]  = _carData.tempStep;
    doc["speedMin"]  = _carData.speedMin;
    doc["speedMax"]  = _carData.speedMax;
    doc["speedStep"] = _carData.speedStep;
    doc["rpmMin"]    = _carData.rpmMin;
    doc["rpmMax"]    = _carData.rpmMax;
    doc["rpmStep"]   = _carData.rpmStep;
    doc["tripMMin"]  = _carData.tripMMin;
    doc["tripMMax"]  = _carData.tripMMax;
    doc["tripMStep"] = _carData.tripMStep;
    doc["lphMin"]    = _carData.lphMin;
    doc["lphMax"]    = _carData.lphMax;
    doc["lphStep"]   = _carData.lphStep;

    // WiFi
    doc["wifiSsid1"] = _carData.wifiSsid1;
    doc["wifiPass1"] = _carData.wifiPass1;
    doc["wifiSsid2"] = _carData.wifiSsid2;
    doc["wifiPass2"] = _carData.wifiPass2;
    doc["wifiSsid3"] = _carData.wifiSsid3;
    doc["wifiPass3"] = _carData.wifiPass3;
    doc["apSsid"]    = _carData.apSsid;
    doc["apPass"]    = _carData.apPass;

    // Bluetooth
    doc["btName"] = _carData.btName;
    doc["btMac"]  = _carData.btMac;
    doc["btPin"]  = _carData.btPin;

    // Калибровка топлива
    doc["fuelCalibrationFactor"]       = _carData.fuelCalibrationFactor;
    doc["lastFullOdoKm"]               = _carData.lastFullOdoKm;
    doc["totalRefuelSinceLastFull"]    = _carData.totalRefuelSinceLastFull;
    doc["calculatedFuelSinceLastFull"] = _carData.calculatedFuelSinceLastFull;
    doc["hasFirstFullTank"]            = _carData.hasFirstFullTank;
    doc["initialOdoKm"]                = _carData.initialOdoKm;
    doc["initialFuel"]                 = _carData.initialFuel;
    doc["tripAutoStopTimeout"]         = _carData.tripAutoStopTimeout;

    // PID-параметры вентиляторов
    doc["fanSetpoint"]   = _carData.fanSetpoint;
    doc["fanKp"]         = _carData.fanKp;
    doc["fanKi"]         = _carData.fanKi;
    doc["fanKd"]         = _carData.fanKd;
    doc["fanAutoMode"]   = _carData.fanAutoMode;
    doc["fanManualPWM"]  = _carData.fanManualPWM;
    doc["climateSetpoint"]   = _carData.climateSetpoint;
    doc["climateAutoMode"]   = _carData.climateAutoMode;
    doc["climateManualPWM"]  = _carData.climateManualPWM;

    // Калибровочные точки печки (новое)
    doc["climateCalibStartPoint"] = _carData.climateCalibStartPoint;
    doc["climateCalibStopPoint"]  = _carData.climateCalibStopPoint;
    doc["climateCalibNoiseLow"]   = _carData.climateCalibNoiseLow;
    doc["climateCalibNoiseHigh"]  = _carData.climateCalibNoiseHigh;
}

// -------------------------------------------------------------------
// Десериализация НАСТРОЕК
// -------------------------------------------------------------------
void StorageManager::deserializeConfig(JsonDocument& doc) {
    _carData.colorCyan   = doc["colorCyan"]   | COLOR_CYAN;
    _carData.colorOrange = doc["colorOrange"] | COLOR_ORANGE;
    _carData.colorRed    = doc["colorRed"]    | COLOR_RED;
    _carData.colorGreen  = doc["colorGreen"]  | COLOR_GREEN;
    _carData.colorBlue   = doc["colorBlue"]   | COLOR_BLUE;
    _carData.colorYellow = doc["colorYellow"] | COLOR_YELLOW;

    _carData.fuelRedThreshold    = doc["fuelRedThreshold"]    | FUEL_RED_THRESHOLD;
    _carData.fuelYellowThreshold = doc["fuelYellowThreshold"] | FUEL_YELLOW_THRESHOLD;
    _carData.battLowThreshold    = doc["battLowThreshold"]    | BATT_LOW_THRESHOLD;
    _carData.battHighThreshold   = doc["battHighThreshold"]   | BATT_HIGH_THRESHOLD;
    _carData.tempCold            = doc["tempCold"]            | TEMP_COLD;
    _carData.tempNormal          = doc["tempNormal"]          | TEMP_NORMAL;
    _carData.tempWarm            = doc["tempWarm"]            | TEMP_WARM;
    _carData.tempHot             = doc["tempHot"]             | TEMP_HOT;
    _carData.speedLow            = doc["speedLow"]            | SPEED_LOW;
    _carData.speedMidLow         = doc["speedMidLow"]         | SPEED_MID_LOW;
    _carData.speedMidHigh        = doc["speedMidHigh"]        | SPEED_MID_HIGH;
    _carData.speedHighLow        = doc["speedHighLow"]        | SPEED_HIGH_LOW;
    _carData.speedHighHigh       = doc["speedHighHigh"]       | SPEED_HIGH_HIGH;
    _carData.speedVHighLow       = doc["speedVHighLow"]       | SPEED_VHIGH_LOW;
    _carData.speedVHighHigh      = doc["speedVHighHigh"]      | SPEED_VHIGH_HIGH;
    _carData.rpmBlueMax          = doc["rpmBlueMax"]          | RPM_BLUE_MAX;
    _carData.rpmCyanMax          = doc["rpmCyanMax"]          | RPM_CYAN_MAX;
    _carData.rpmYellowMax        = doc["rpmYellowMax"]        | RPM_YELLOW_MAX;
    _carData.lphBlueMax          = doc["lphBlueMax"]          | LPH_BLUE_MAX;
    _carData.lphCyanMax          = doc["lphCyanMax"]          | LPH_CYAN_MAX;
    _carData.lphYellowMax        = doc["lphYellowMax"]        | LPH_YELLOW_MAX;

    _carData.fuelMin   = doc["fuelMin"]   | FUEL_MIN;
    _carData.fuelMax   = doc["fuelMax"]   | FUEL_MAX;
    _carData.fuelStep  = doc["fuelStep"]  | FUEL_STEP;
    _carData.rangeMin  = doc["rangeMin"]  | RANGE_MIN;
    _carData.rangeMax  = doc["rangeMax"]  | RANGE_MAX;
    _carData.rangeStep = doc["rangeStep"] | RANGE_STEP;
    _carData.battMin   = doc["battMin"]   | BATT_MIN;
    _carData.battMax   = doc["battMax"]   | BATT_MAX;
    _carData.battStep  = doc["battStep"]  | BATT_STEP;
    _carData.tempMin   = doc["tempMin"]   | TEMP_MIN;
    _carData.tempMax   = doc["tempMax"]   | TEMP_MAX;
    _carData.tempStep  = doc["tempStep"]  | TEMP_STEP;
    _carData.speedMin  = doc["speedMin"]  | SPEED_MIN;
    _carData.speedMax  = doc["speedMax"]  | SPEED_MAX;
    _carData.speedStep = doc["speedStep"] | SPEED_STEP;
    _carData.rpmMin    = doc["rpmMin"]    | RPM_MIN;
    _carData.rpmMax    = doc["rpmMax"]    | RPM_MAX;
    _carData.rpmStep   = doc["rpmStep"]   | RPM_STEP;
    _carData.tripMMin  = doc["tripMMin"]  | TRIPM_MIN;
    _carData.tripMMax  = doc["tripMMax"]  | TRIPM_MAX;
    _carData.tripMStep = doc["tripMStep"] | TRIPM_STEP;
    _carData.lphMin    = doc["lphMin"]    | LPH_MIN;
    _carData.lphMax    = doc["lphMax"]    | LPH_MAX;
    _carData.lphStep   = doc["lphStep"]   | LPH_STEP;

    strlcpy(_carData.wifiSsid1, doc["wifiSsid1"] | "Tri-Al(m)", sizeof(_carData.wifiSsid1));
    strlcpy(_carData.wifiPass1, doc["wifiPass1"] | "Ford-Fiona", sizeof(_carData.wifiPass1));
    strlcpy(_carData.wifiSsid2, doc["wifiSsid2"] | "Tri-AL", sizeof(_carData.wifiSsid2));
    strlcpy(_carData.wifiPass2, doc["wifiPass2"] | "Aq1Sw2De3Fr4", sizeof(_carData.wifiPass2));
    strlcpy(_carData.wifiSsid3, doc["wifiSsid3"] | "KbKb", sizeof(_carData.wifiSsid3));
    strlcpy(_carData.wifiPass3, doc["wifiPass3"] | "1234567890", sizeof(_carData.wifiPass3));
    strlcpy(_carData.apSsid,    doc["apSsid"]    | "Fiona-WiFi", sizeof(_carData.apSsid));
    strlcpy(_carData.apPass,    doc["apPass"]    | "Ford-Fiona", sizeof(_carData.apPass));

    strlcpy(_carData.btName, doc["btName"] | "Fiona-BT", sizeof(_carData.btName));
    strlcpy(_carData.btMac,  doc["btMac"]  | "00:1D:A5:07:05:17", sizeof(_carData.btMac));
    strlcpy(_carData.btPin,  doc["btPin"]  | "1234", sizeof(_carData.btPin));

    _carData.fuelCalibrationFactor        = doc["fuelCalibrationFactor"]        | 1.0f;
    _carData.lastFullOdoKm                = doc["lastFullOdoKm"]                | 0;
    _carData.totalRefuelSinceLastFull     = doc["totalRefuelSinceLastFull"]     | 0.0f;
    _carData.calculatedFuelSinceLastFull  = doc["calculatedFuelSinceLastFull"]  | 0.0f;
    _carData.hasFirstFullTank             = doc["hasFirstFullTank"]             | false;
    _carData.initialOdoKm                 = doc["initialOdoKm"]                 | 0;
    _carData.initialFuel                  = doc["initialFuel"]                  | 0.0f;
    _carData.tripAutoStopTimeout          = doc["tripAutoStopTimeout"]          | 60;

    _carData.fanSetpoint   = doc["fanSetpoint"]   | 95.5f;
    _carData.fanKp         = doc["fanKp"]         | 30.0f;
    _carData.fanKi         = doc["fanKi"]         | 2.0f;
    _carData.fanKd         = doc["fanKd"]         | 5.0f;
    _carData.fanAutoMode   = doc["fanAutoMode"]   | true;
    _carData.fanManualPWM  = doc["fanManualPWM"]  | 0;
    _carData.climateSetpoint  = doc["climateSetpoint"]  | 22.0f;
    _carData.climateAutoMode  = doc["climateAutoMode"]  | true;
    _carData.climateManualPWM = doc["climateManualPWM"] | 0;

    // Калибровки печки (новое)
    _carData.climateCalibStartPoint = doc["climateCalibStartPoint"] | 0;
    _carData.climateCalibStopPoint  = doc["climateCalibStopPoint"]  | 0;
    _carData.climateCalibNoiseLow   = doc["climateCalibNoiseLow"]   | 0;
    _carData.climateCalibNoiseHigh  = doc["climateCalibNoiseHigh"]  | 0;
}

// -------------------------------------------------------------------
// Сериализация СОСТОЯНИЯ (топливо, одометр, поездки, климат)
// -------------------------------------------------------------------
void StorageManager::serializeState(JsonDocument& doc) {
    // Топливо теперь хранится в миллилитрах
    doc["fuelValueML"]       = _carData.fuelValueML;
    doc["odoKm"]             = _carData.odoKm;
    doc["tripValue"]         = _carData.tripValue;
    doc["tripMValue"]        = _carData.tripMValue;
    doc["tripState"]         = _carData.tripState;
    doc["calibrationNeeded"] = _carData.calibrationNeeded;
    doc["lastFullOdoKm"]     = _carData.lastFullOdoKm;
    doc["totalRefuelSinceLastFull"]   = _carData.totalRefuelSinceLastFull;
    doc["calculatedFuelSinceLastFull"]= _carData.calculatedFuelSinceLastFull;
    doc["hasFirstFullTank"]  = _carData.hasFirstFullTank;
    doc["lastManualStopTime"]= _carData.lastManualStopTime;
    doc["sleepTimeout"]      = _carData.sleepTimeout;
    doc["tripDistanceKm"]    = _carData.tripDistanceKm;
    doc["tripFuelUsed"]      = _carData.tripFuelUsed;
    doc["tripPauseValue"]    = _carData.tripPauseValue;

    doc["tripStatStartTime"] = _carData.tripStatStartTime;
    doc["tripStatIsManual"]  = _carData.tripStatIsManual;
    doc["tripStatDuration"]  = _carData.tripStatDuration;
    doc["tripStatPauseTime"] = _carData.tripStatPauseTime;
    doc["tripStatPauseCount"]= _carData.tripStatPauseCount;
    doc["tripStatDistance"]  = _carData.tripStatDistance;
    doc["tripStatFuelUsed"]  = _carData.tripStatFuelUsed;
    doc["tripStatMaxSpeed"]  = _carData.tripStatMaxSpeed;
    doc["tripStatMaxLPH"]    = _carData.tripStatMaxLPH;
    doc["tripStatPending"]   = _carData.tripStatPending;

    doc["dayStatDate"]           = _carData.dayStatDate;
    doc["dayStatValid"]          = _carData.dayStatValid;
    doc["dayStatEngineSeconds"]  = _carData.dayStatEngineSeconds;
    doc["dayStatDistance"]       = _carData.dayStatDistance;
    doc["dayStatFuelUsed"]       = _carData.dayStatFuelUsed;
    doc["dayStatMaxSpeed"]       = _carData.dayStatMaxSpeed;
    doc["dayStatMaxLPH"]         = _carData.dayStatMaxLPH;
    doc["dayStatPending"]        = _carData.dayStatPending;

    // Вентиляторы и климат
    doc["fanAutoMode"]       = _carData.fanAutoMode;
    doc["fanManualPWM"]      = _carData.fanManualPWM;
    doc["fanCurrentPWM1"]    = _carData.fanCurrentPWM1;
    doc["fanCurrentPWM2"]    = _carData.fanCurrentPWM2;
    doc["climateAutoMode"]   = _carData.climateAutoMode;
    doc["climateManualPWM"]  = _carData.climateManualPWM;
    doc["climateCurrentPWM"] = _carData.climateCurrentPWM;
}

// -------------------------------------------------------------------
// Десериализация СОСТОЯНИЯ
// -------------------------------------------------------------------
void StorageManager::deserializeState(JsonDocument& doc) {
    _carData.fuelValueML        = doc["fuelValueML"]          | 50000;
    _carData.odoKm              = doc["odoKm"]              | 200000;
    _carData.tripValue          = doc["tripValue"]          | 0;
    _carData.tripMValue         = doc["tripMValue"]         | 0.0f;
    _carData.tripState          = doc["tripState"]          | false;
    _carData.calibrationNeeded  = doc["calibrationNeeded"]  | false;
    _carData.lastFullOdoKm      = doc["lastFullOdoKm"]      | 0;
    _carData.totalRefuelSinceLastFull   = doc["totalRefuelSinceLastFull"]   | 0.0f;
    _carData.calculatedFuelSinceLastFull= doc["calculatedFuelSinceLastFull"]| 0.0f;
    _carData.hasFirstFullTank   = doc["hasFirstFullTank"]   | false;
    _carData.lastManualStopTime = doc["lastManualStopTime"] | 0;
    _carData.sleepTimeout       = doc["sleepTimeout"]       | 15;
    _carData.tripDistanceKm     = doc["tripDistanceKm"]     | 0.0f;
    _carData.tripFuelUsed       = doc["tripFuelUsed"]       | 0.0f;
    _carData.tripPauseValue     = doc["tripPauseValue"]     | 0;

    _carData.tripStatStartTime  = doc["tripStatStartTime"]  | 0;
    _carData.tripStatIsManual   = doc["tripStatIsManual"]   | false;
    _carData.tripStatDuration   = doc["tripStatDuration"]   | 0;
    _carData.tripStatPauseTime  = doc["tripStatPauseTime"]  | 0;
    _carData.tripStatPauseCount = doc["tripStatPauseCount"] | 0;
    _carData.tripStatDistance   = doc["tripStatDistance"]   | 0;
    _carData.tripStatFuelUsed   = doc["tripStatFuelUsed"]   | 0.0f;
    _carData.tripStatMaxSpeed   = doc["tripStatMaxSpeed"]   | 0;
    _carData.tripStatMaxLPH     = doc["tripStatMaxLPH"]     | 0;
    _carData.tripStatPending    = doc["tripStatPending"]    | false;

    _carData.dayStatDate          = doc["dayStatDate"]          | 0;
    _carData.dayStatValid         = doc["dayStatValid"]         | false;
    _carData.dayStatEngineSeconds = doc["dayStatEngineSeconds"] | 0;
    _carData.dayStatDistance      = doc["dayStatDistance"]      | 0;
    _carData.dayStatFuelUsed      = doc["dayStatFuelUsed"]      | 0.0f;
    _carData.dayStatMaxSpeed      = doc["dayStatMaxSpeed"]      | 0;
    _carData.dayStatMaxLPH        = doc["dayStatMaxLPH"]        | 0;
    _carData.dayStatPending       = doc["dayStatPending"]       | false;

    _carData.fanAutoMode        = doc["fanAutoMode"]        | true;
    _carData.fanManualPWM       = doc["fanManualPWM"]       | 0;
    _carData.fanCurrentPWM1     = doc["fanCurrentPWM1"]     | 0;
    _carData.fanCurrentPWM2     = doc["fanCurrentPWM2"]     | 0;
    _carData.climateAutoMode    = doc["climateAutoMode"]    | true;
    _carData.climateManualPWM   = doc["climateManualPWM"]   | 0;
    _carData.climateCurrentPWM  = doc["climateCurrentPWM"]  | 0;
}