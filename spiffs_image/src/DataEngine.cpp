#include "DataEngine.h"

#define MAF_TO_LPH 0.32653f

// --------------------- Датчик освещённости ---------------------
static uint32_t lightAccumulator = 0;
static uint16_t lightSampleCount = 0;
static uint8_t  lightLevelPercent = 0;
static uint16_t lightRawAvg = 0;
static uint32_t lastLightReportTime = 0;

void DataEngine::updateLight() {
    int raw = analogRead(36);
    lightAccumulator += raw;
    lightSampleCount++;

    uint32_t now = millis();
    if (now - lastLightReportTime >= 1000) {
        if (lightSampleCount > 0) {
            uint16_t avg = lightAccumulator / lightSampleCount;
            lightRawAvg = avg;
            if (avg >= 4095) lightLevelPercent = 100;
            else if (avg <= 0) lightLevelPercent = 0;
            else lightLevelPercent = (uint8_t)((avg * 100) / 4095);
        }
        //Serial.printf("[Light] Sensor: %d%% (raw=%d)\n", lightLevelPercent, lightRawAvg);
        lightAccumulator = 0;
        lightSampleCount = 0;
        lastLightReportTime = now;
    }
}

uint8_t DataEngine::getLightLevel() {
    return lightLevelPercent;
}

uint16_t DataEngine::getLightRaw() {
    return lightRawAvg;
}

// --------------------- Обновление данных OBD ---------------------
void DataEngine::updateMAF(CarData* data, float value) {
    if (data->mafValue != value) {
        data->mafValue = value;
        data->mafDirty = true;
    }
    float lph = value * MAF_TO_LPH;
    if (data->lphValue != lph) {
        data->lphValue = lph;
        data->lphDirty = true;
    }
}

void DataEngine::updateRPM(CarData* data, float value) {
    int rpm = (int)value;
    if (data->rpmValue != rpm) {
        data->rpmValue = rpm;
        data->rpmDirty = true;
    }
}

void DataEngine::updateSpeed(CarData* data, float value) {
    int speed = (int)value;
    if (data->speedValue != speed) {
        data->speedValue = speed;
        data->speedDirty = true;
    }
}

void DataEngine::updateCoolantTemp(CarData* data, float value) {
    int temp = (int)value;
    if (data->tempValue != temp) {
        data->tempValue = temp;
        data->tempDirty = true;
    }
}

void DataEngine::updateVoltage(CarData* data, float value) {
    if (data->batValue != value) {
        data->batValue = value;
        data->batDirty = true;
    }
}

void DataEngine::updateThrottle(CarData* data, float value) {
    if (data->throttlePos != value) {
        data->throttlePos = value;
        data->throttleDirty = true;
    }
}

void DataEngine::updateThrottleRel(CarData* data, float value) {
    if (data->throttleRelPos != value) {
        data->throttleRelPos = value;
        data->throttleRelDirty = true;
    }
}

DataEngine::DataEngine(OBDEngine& obd, CarData& data)
    : _obd(obd), _carData(data) {}

void DataEngine::begin() {
    _pids[0] = {"0110", 300, 1, 0, updateMAF};
    _pids[1] = {"010D", 250, 2, 0, updateSpeed};
    _pids[2] = {"010C", 250, 3, 0, updateRPM};
    _pids[3] = {"0105", 5000, 4, 0, updateCoolantTemp};
    _pids[4] = {"ATRV", 5000, 4, 0, updateVoltage};
    _pids[5] = {"0111", 300, 2, 0, updateThrottle};
    _pids[6] = {"0145", 300, 2, 0, updateThrottleRel};
}

void DataEngine::process() {
    uint32_t now = millis();
    int selectedIdx = -1;
    uint32_t maxDelay = 0;

    for (int i = 0; i < PID_COUNT; i++) {
        uint32_t elapsed = now - _pids[i].lastRequest;
        if (elapsed >= _pids[i].interval) {
            if (elapsed > maxDelay) {
                maxDelay = elapsed;
                selectedIdx = i;
            }
        }
    }

    if (selectedIdx != -1) {
        unsigned long elapsed;
        bool success;
        float value = _obd.readPIDWithTime(_pids[selectedIdx].command, elapsed, success);

        if (success) {
            _pids[selectedIdx].lastRequest = now;
            if (strcmp(_pids[selectedIdx].command, "ATRV") == 0) {
                if (value >= 8.0f && value <= 16.0f) {
                    _pids[selectedIdx].updateCarData(&_carData, value);
                }
            } else {
                _pids[selectedIdx].updateCarData(&_carData, value);
            }
        }
    }
}