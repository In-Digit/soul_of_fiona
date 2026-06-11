#include "OBDEngine.h"
#include <BluetoothSerial.h>
#include <esp_spp_api.h>
#include <math.h>

OBDEngine::OBDEngine() : _connected(false), _debugMode(false), _emulationMode(false),
    _lastReconnectAttempt(0), _lastMac(""), _lastPin(""),
    _emuLastUpdate(0), _emuRPM(0), _emuSpeed(0), _emuMAF(0), _emuLPH(0), _emuVoltage(12.5),
    _emuCoolantTemp(20), _emuIntakeTemp(15), _emuLoad(0), _emuSTFT(0), _emuLTFT(0), _emuThrottle(0), _emuThrottleRel(0) {
    
    _emuTimers[0] = {"0110", 300, 0, 0.0f};
    _emuTimers[1] = {"010D", 250, 0, 0.0f};
    _emuTimers[2] = {"010C", 250, 0, 0.0f};
    _emuTimers[3] = {"0105", 5000, 0, 0.0f};
    _emuTimers[4] = {"ATRV", 5000, 0, 0.0f};
    _emuTimers[5] = {"0111", 300, 0, 0.0f};
    _emuTimers[6] = {"0145", 300, 0, 0.0f};
}

bool OBDEngine::begin() {
    if (_emulationMode) {
        Serial.println("[OBD] Emulation mode active.");
        _connected = true;
        return true;
    }
    return initBluetooth();
}

bool OBDEngine::initBluetooth() {
    if (_debugMode) Serial.println("[OBD] Init Bluetooth...");
    if (_serialBT.begin("ESP32-OBD", true)) {
        if (_debugMode) Serial.println("[OBD] Bluetooth OK.");
        return true;
    }
    if (_debugMode) Serial.println("[OBD] Bluetooth init failed.");
    return false;
}

bool OBDEngine::connectWithConfig(const char* macStr, const char* pinStr) {
    if (_emulationMode) { _connected = true; return true; }
    if (macStr) _lastMac = String(macStr);
    if (pinStr) _lastPin = String(pinStr);
    if (!connectToELM327(macStr, pinStr)) { if (_debugMode) Serial.println("[OBD] Connection failed."); return false; }
    if (!configureELM327()) { if (_debugMode) Serial.println("[OBD] Config failed."); _serialBT.disconnect(); return false; }
    if (!testECUConnection()) { if (_debugMode) Serial.println("[OBD] ECU not responding."); _serialBT.disconnect(); return false; }
    _connected = true;
    if (_debugMode) Serial.println("[OBD] Connected.");
    return true;
}

void OBDEngine::reconnectIfNeeded() {
    if (_emulationMode || _connected) return;
    uint32_t now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL) return;
    _lastReconnectAttempt = now;
    Serial.println("[OBD] Reconnecting...");
    if (connectWithConfig(_lastMac.c_str(), _lastPin.c_str())) Serial.println("[OBD] Reconnected.");
    else Serial.println("[OBD] Reconnect failed.");
}

bool OBDEngine::connectToELM327(const char* macStr, const char* userPin) {
    uint8_t mac[6] = {0x00, 0x1D, 0xA5, 0x07, 0x05, 0x17};
    if (macStr != nullptr && strlen(macStr) > 0) {
        if (!parseMacAddress(macStr, mac)) { if (_debugMode) Serial.println("[OBD] Invalid MAC, using default."); }
    }
    if (_debugMode) {
        Serial.print("[OBD] Connecting to ");
        for (int i = 0; i < 6; i++) { Serial.printf("%02X", mac[i]); if (i < 5) Serial.print(":"); }
        Serial.println();
    }
    delay(3000);
    const int MAX_ATTEMPTS = 5;
    const char* pins[MAX_ATTEMPTS] = {"0000", userPin, "1234", "", nullptr};
    int pinCount = 0;
    while (pinCount < MAX_ATTEMPTS && pins[pinCount] != nullptr) pinCount++;
    for (int i = 0; i < pinCount; i++) {
        _serialBT.setPin(pins[i]); bool connected = false;
        if (_debugMode) Serial.printf("[OBD] Attempt %d with PIN '%s'\n", i+1, pins[i]);
        connected = (i == 0) ? _serialBT.connect(mac, 0, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE) : _serialBT.connect(mac);
        delay(1000);
        if (connected && _serialBT.connected()) {
            while (_serialBT.available()) _serialBT.read();
            if (_debugMode) Serial.printf("[OBD] Connected on attempt %d\n", i+1);
            return true;
        } else { if (_debugMode) Serial.printf("[OBD] Attempt %d failed\n", i+1); _serialBT.disconnect(); delay(2000); }
    }
    if (_debugMode) Serial.println("[OBD] All attempts failed.");
    return false;
}

bool OBDEngine::configureELM327() {
    while (_serialBT.available()) _serialBT.read();
    String response = sendCommand("ATZ", 2000);
    if (response.indexOf("ELM327") == -1) { if (_debugMode) Serial.println("[OBD] ATZ failed."); return false; }
    delay(100); sendCommand("ATE0", 500); delay(50);
    sendCommand("ATS0", 500); delay(50); sendCommand("ATSP0", 500);
    if (_debugMode) Serial.println("[OBD] ELM327 configured.");
    return true;
}

bool OBDEngine::testECUConnection() {
    String response = sendCommand("0100", 2000);
    if (response.length() == 0 || response.indexOf("NO DATA") != -1) { if (_debugMode) Serial.println("[OBD] ECU not responding."); return false; }
    if (_debugMode) Serial.println("[OBD] ECU OK.");
    return true;
}

String OBDEngine::sendCommand(const char* cmd, unsigned long timeout) {
    if (_emulationMode) return "";
    if (!_serialBT.connected()) { if (_debugMode) Serial.printf("[OBD] Not connected, can't send %s\n", cmd); return ""; }
    while (_serialBT.available()) _serialBT.read();
    if (_debugMode) Serial.printf("[OBD] >> %s\n", cmd);
    _serialBT.println(cmd);
    unsigned long start = millis(); String response = "";
    while (millis() - start < timeout) { if (_serialBT.available()) { char c = _serialBT.read(); response += c; if (c == '>') break; } delay(1); }
    if (_debugMode && response.length() > 0) { response.trim(); Serial.printf("[OBD] << %s\n", response.c_str()); }
    return response;
}

float OBDEngine::readPIDWithTime(const char* pid, unsigned long& elapsedMs, bool& success) {
    if (_emulationMode) {
        elapsedMs = 5; success = true;
        for (int i = 0; i < 7; i++) {
            if (strcmp(_emuTimers[i].pid, pid) == 0) {
                uint32_t now = millis();
                if (now - _emuTimers[i].lastUpdate >= _emuTimers[i].interval) {
                    float newVal = emulatePID(pid);
                    _emuTimers[i].lastValue = newVal;
                    _emuTimers[i].lastUpdate = now;
                    return newVal;
                } else return _emuTimers[i].lastValue;
            }
        }
        return emulatePID(pid);
    }
    if (!_connected) { elapsedMs = 0; success = false; return 0.0f; }
    unsigned long start = millis();
    String response = sendCommand(pid, 500);
    elapsedMs = millis() - start;
    if (response.length() == 0) { success = false; return 0.0f; }
    float value = FordFocus2001_Parser::parseELM327Response(response, pid);
    success = true;
    return value;
}

float OBDEngine::queryVoltageATRV() {
    unsigned long elapsed; bool success;
    return readPIDWithTime("ATRV", elapsed, success);
}

bool OBDEngine::parseMacAddress(const char* macStr, uint8_t* mac) {
    int parts[6];
    if (sscanf(macStr, "%02x:%02x:%02x:%02x:%02x:%02x", &parts[0],&parts[1],&parts[2],&parts[3],&parts[4],&parts[5]) == 6) {
        for (int i=0;i<6;i++) mac[i] = (uint8_t)parts[i]; return true;
    }
    return false;
}

void OBDEngine::updateEmulation() {}

float OBDEngine::emulatePID(const char* pid) {
    uint32_t now = millis();
    if (strcmp(pid, "010C") == 0) return 800 + 2700 * (0.5f + 0.5f * sin(now * 0.001f));
    if (strcmp(pid, "010D") == 0) return 120 * (0.5f + 0.5f * sin(now * 0.0005f));
    if (strcmp(pid, "0110") == 0) return 2.0f + 8.0f * (0.5f + 0.5f * sin(now * 0.002f));
    if (strcmp(pid, "0105") == 0) return 70.0f + 35.0f * (0.5f + 0.5f * sin(now * 0.0001f));
    if (strcmp(pid, "010F") == 0) return 15.0f + 25.0f * (0.5f + 0.5f * sin(now * 0.00005f));
    if (strcmp(pid, "ATRV") == 0) return 13.5f + 1.0f * sin(now * 0.003f);
    if (strcmp(pid, "0111") == 0) return 15.0f + 40.0f * (0.5f + 0.5f * sin(now * 0.0008f));
    if (strcmp(pid, "0145") == 0) return 10.0f + 30.0f * (0.5f + 0.5f * sin(now * 0.0006f));
    return 0.0f;
}

float OBDEngine::emulateVoltage() { return _emuVoltage; }