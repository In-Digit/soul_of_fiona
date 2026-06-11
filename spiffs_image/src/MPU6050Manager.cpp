#include "MPU6050Manager.h"
#include "Protocol.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

MPU6050Manager::MPU6050Manager() : _mpu(Wire), _calibrated(false), _calibStatus(CALIB_IDLE) {
    memset(_accelOffsets, 0, sizeof(_accelOffsets));
    memset(_gyroOffsets, 0, sizeof(_gyroOffsets));
    memset(_rotationMatrix, 0, sizeof(_rotationMatrix));
    for (int i = 0; i < 3; i++) _rotationMatrix[i][i] = 1.0f;
}

bool MPU6050Manager::begin() {
    Wire.begin(21, 22);
    byte status = _mpu.begin();
    if (status != 0) {
        _calibStatus = CALIB_ERR_NO_SENSOR;
        return false;
    }
    _mpu.calcOffsets();
    if (loadFromFlash()) {
        _calibrated = true;
        _calibStatus = CALIB_SUCCESS;
    } else {
        _calibrated = false;
        _calibStatus = CALIB_IDLE;
    }
    return true;
}

void MPU6050Manager::update() {
    if (!_calibrated) return;

    _mpu.update();

    float ax = _mpu.getAccX() - _accelOffsets[0];
    float ay = _mpu.getAccY() - _accelOffsets[1];
    float az = _mpu.getAccZ() - _accelOffsets[2];
    float gx = _mpu.getGyroX() - _gyroOffsets[0];
    float gy = _mpu.getGyroY() - _gyroOffsets[1];
    float gz = _mpu.getGyroZ() - _gyroOffsets[2];

    _ax = _rotationMatrix[0][0] * ax + _rotationMatrix[0][1] * ay + _rotationMatrix[0][2] * az;
    _ay = _rotationMatrix[1][0] * ax + _rotationMatrix[1][1] * ay + _rotationMatrix[1][2] * az;
    _az = _rotationMatrix[2][0] * ax + _rotationMatrix[2][1] * ay + _rotationMatrix[2][2] * az;

    _gx = _rotationMatrix[0][0] * gx + _rotationMatrix[0][1] * gy + _rotationMatrix[0][2] * gz;
    _gy = _rotationMatrix[1][0] * gx + _rotationMatrix[1][1] * gy + _rotationMatrix[1][2] * gz;
    _gz = _rotationMatrix[2][0] * gx + _rotationMatrix[2][1] * gy + _rotationMatrix[2][2] * gz;

    _roll = _mpu.getAngleX();
    _pitch = _mpu.getAngleY();

    static uint32_t lastLog = 0;
    if (millis() - lastLog >= 5000) {
        lastLog = millis();
        Serial.printf("[MPU] Accel: %.2f %.2f %.2f m/s² | Gyro: %.2f %.2f %.2f °/s | Tilt: %.1f %.1f°\n",
                      _ax, _ay, _az, _gx, _gy, _gz, _roll, _pitch);
    }
}

void MPU6050Manager::calibrateAlignment() {
    if (_calibStatus == CALIB_PHASE1_ACTIVE || _calibStatus == CALIB_PHASE2_ACTIVE) return;
    _calibStatus = CALIB_PHASE1_ACTIVE;
}

void MPU6050Manager::processCalibration() {
    static uint32_t phaseStart = 0;
    static float staticSumX = 0, staticSumY = 0, staticSumZ = 0;
    static int staticSamples = 0;

    if (_calibStatus == CALIB_PHASE1_ACTIVE) {
        if (staticSamples == 0) {
            phaseStart = millis();
            Serial.println("[MPU] Phase 1: keep car still...");
        }
        _mpu.update();
        staticSumX += _mpu.getAccX();
        staticSumY += _mpu.getAccY();
        staticSumZ += _mpu.getAccZ();
        staticSamples++;
        if (staticSamples >= 100) {
            float avgX = staticSumX / staticSamples;
            float avgY = staticSumY / staticSamples;
            float avgZ = staticSumZ / staticSamples;
            computeRotationMatrix(avgX, avgY, avgZ);
            _accelOffsets[0] = avgX;
            _accelOffsets[1] = avgY;
            _accelOffsets[2] = avgZ - 1.0f;
            _gyroOffsets[0] = _mpu.getGyroX();
            _gyroOffsets[1] = _mpu.getGyroY();
            _gyroOffsets[2] = _mpu.getGyroZ();
            _calibStatus = CALIB_PHASE2_ACTIVE;
            Serial.println("[MPU] Phase 1 OK. Phase 2: accelerate, coast, then brake firmly...");
        }
        return;
    }

    static uint32_t phase2Start = 0;
    static float bestMag = 0.4f;
    static float bestX, bestY, bestZ;
    static bool found = false;

    if (_calibStatus == CALIB_PHASE2_ACTIVE) {
        if (phase2Start == 0) {
            phase2Start = millis();
            bestMag = 0.4f;
            found = false;
        }
        if (millis() - phase2Start > 15000) {
            if (found) {
                float norm = sqrt(bestX * bestX + bestY * bestY);
                if (norm > 0.001f) {
                    float fx = bestX / norm, fy = bestY / norm;
                    float angle = atan2(fy, fx);
                    float c = cos(-angle), s = sin(-angle);
                    float rotZ[3][3] = {{c, -s, 0}, {s, c, 0}, {0, 0, 1}};
                    float newMat[3][3];
                    for (int i = 0; i < 3; i++)
                        for (int j = 0; j < 3; j++) {
                            newMat[i][j] = 0;
                            for (int k = 0; k < 3; k++)
                                newMat[i][j] += rotZ[i][k] * _rotationMatrix[k][j];
                        }
                    memcpy(_rotationMatrix, newMat, sizeof(_rotationMatrix));
                    _calibrated = true;
                    _calibStatus = CALIB_SUCCESS;
                    saveToFlash();
                    Serial.println("[MPU] Calibration completed successfully");
                }
            } else {
                _calibStatus = CALIB_ERR_PHASE2_FAIL;
                Serial.println("[MPU] Phase 2 failed: no strong acceleration detected");
            }
            return;
        }
        _mpu.update();
        float ax = _mpu.getAccX() - _accelOffsets[0];
        float ay = _mpu.getAccY() - _accelOffsets[1];
        float az = _mpu.getAccZ() - _accelOffsets[2];
        float mag = sqrt(ax * ax + ay * ay + az * az);
        if (mag > bestMag) {
            bestMag = mag;
            bestX = ax;
            bestY = ay;
            bestZ = az;
            found = true;
        }
    }
}

void MPU6050Manager::computeRotationMatrix(float gx, float gy, float gz) {
    float norm = sqrt(gx * gx + gy * gy + gz * gz);
    if (norm < 0.001f) return;
    gx /= norm; gy /= norm; gz /= norm;

    float crossX = gy * 1.0f - gz * 0.0f;
    float crossY = gz * 0.0f - gx * 1.0f;
    float crossZ = gx * 0.0f - gy * 0.0f;
    float cosTheta = gz;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    if (sinTheta < 0.001f) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                _rotationMatrix[i][j] = (i == j) ? 1.0f : 0.0f;
        return;
    }
    crossX /= sinTheta; crossY /= sinTheta; crossZ /= sinTheta;

    float c = cosTheta, s = sinTheta, t = 1.0f - c;
    float x = crossX, y = crossY, z = crossZ;
    _rotationMatrix[0][0] = t * x * x + c;
    _rotationMatrix[0][1] = t * x * y - s * z;
    _rotationMatrix[0][2] = t * x * z + s * y;
    _rotationMatrix[1][0] = t * x * y + s * z;
    _rotationMatrix[1][1] = t * y * y + c;
    _rotationMatrix[1][2] = t * y * z - s * x;
    _rotationMatrix[2][0] = t * x * z - s * y;
    _rotationMatrix[2][1] = t * y * z + s * x;
    _rotationMatrix[2][2] = t * z * z + c;
}

void MPU6050Manager::saveToFlash() {
    StaticJsonDocument<512> doc;
    doc["ax_off"] = _accelOffsets[0];
    doc["ay_off"] = _accelOffsets[1];
    doc["az_off"] = _accelOffsets[2];
    doc["gx_off"] = _gyroOffsets[0];
    doc["gy_off"] = _gyroOffsets[1];
    doc["gz_off"] = _gyroOffsets[2];
    JsonArray arr = doc.createNestedArray("R");
    for (int i = 0; i < 3; i++) {
        JsonArray row = arr.createNestedArray();
        for (int j = 0; j < 3; j++)
            row.add(_rotationMatrix[i][j]);
    }

    File f = LittleFS.open("/mpu.json", "w");
    if (f) {
        // serializeJson возвращает количество записанных байт, 0 означает ошибку
        if (serializeJson(doc, f) == 0) {
            Serial.println("[MPU] Failed to write calibration data");
        } else {
            Serial.println("[MPU] Calibration saved");
        }
        f.close();   // теперь закрываем в любом случае
    } else {
        Serial.println("[MPU] Failed to open calibration file for writing");
    }
}

bool MPU6050Manager::loadFromFlash() {
    if (!LittleFS.exists("/mpu.json")) return false;
    File f = LittleFS.open("/mpu.json", "r");
    if (!f) return false;
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.println("[MPU] Failed to parse calibration file");
        return false;
    }

    _accelOffsets[0] = doc["ax_off"] | 0.0f;
    _accelOffsets[1] = doc["ay_off"] | 0.0f;
    _accelOffsets[2] = doc["az_off"] | 0.0f;
    _gyroOffsets[0] = doc["gx_off"] | 0.0f;
    _gyroOffsets[1] = doc["gy_off"] | 0.0f;
    _gyroOffsets[2] = doc["gz_off"] | 0.0f;
    JsonArray arr = doc["R"].as<JsonArray>();
    if (arr.size() == 3) {
        for (int i = 0; i < 3; i++) {
            JsonArray row = arr[i].as<JsonArray>();
            if (row.size() == 3)
                for (int j = 0; j < 3; j++)
                    _rotationMatrix[i][j] = row[j].as<float>();
        }
    }
    return true;
}

MPU6050Manager mpuManager;   // глобальный экземпляр