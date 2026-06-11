#pragma once
#include <BluetoothSerial.h>
#include <esp_spp_api.h>
#include "FordFocus2001_Parser.h"

class OBDEngine {
public:
    OBDEngine();
    
    bool begin();
    bool connectWithConfig(const char* macStr, const char* pinStr);
    void reconnectIfNeeded();
    float readPIDWithTime(const char* pid, unsigned long& elapsedMs, bool& success);
    float queryVoltageATRV();
    bool isConnected() const { return _connected; }
    String sendCommand(const char* cmd, unsigned long timeout = 500);

    void setEmulationMode(bool enable) { _emulationMode = enable; }
    bool isEmulationMode() const { return _emulationMode; }

private:
    BluetoothSerial _serialBT;
    bool _connected;
    bool _debugMode;
    bool _emulationMode;

    uint32_t _lastReconnectAttempt;
    static const uint32_t RECONNECT_INTERVAL = 30000;
    String _lastMac;
    String _lastPin;

    struct {
        uint8_t mac[6] = {0x00, 0x1D, 0xA5, 0x07, 0x05, 0x17};
    } _config;

    bool initBluetooth();
    bool connectToELM327(const char* macStr, const char* userPin);
    bool configureELM327();
    bool testECUConnection();
    bool parseMacAddress(const char* macStr, uint8_t* mac);

    struct EmuTimer {
        const char* pid;
        uint32_t interval;
        uint32_t lastUpdate;
        float lastValue;
    };
    EmuTimer _emuTimers[7];
    uint32_t _emuLastUpdate;
    float _emuRPM, _emuSpeed, _emuMAF, _emuLPH, _emuVoltage;
    float _emuCoolantTemp, _emuIntakeTemp, _emuLoad, _emuSTFT, _emuLTFT, _emuThrottle, _emuThrottleRel;

    void updateEmulation();
    float emulatePID(const char* pid);
    float emulateVoltage();
};