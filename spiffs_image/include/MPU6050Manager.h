#ifndef MPU6050_MANAGER_H
#define MPU6050_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>

class MPU6050Manager {
public:
    MPU6050Manager();
    bool begin();
    void update();
    void calibrateAlignment();
    void processCalibration();   // вызывается в dataTask
    bool isCalibrated() const { return _calibrated; }
    uint8_t getCalibStatus() const { return _calibStatus; }

    float getAccelX() const { return _ax; }
    float getAccelY() const { return _ay; }
    float getAccelZ() const { return _az; }
    float getGyroX() const  { return _gx; }
    float getGyroY() const  { return _gy; }
    float getGyroZ() const  { return _gz; }
    float getRoll() const   { return _roll; }
    float getPitch() const  { return _pitch; }

private:
    MPU6050 _mpu;
    bool _calibrated;
    uint8_t _calibStatus;

    float _accelOffsets[3];
    float _gyroOffsets[3];
    float _rotationMatrix[3][3];

    float _ax, _ay, _az;
    float _gx, _gy, _gz;
    float _roll, _pitch;

    void computeRotationMatrix(float gx, float gy, float gz);
    void saveToFlash();
    bool loadFromFlash();
};

#endif