// main.cpp — полный файл с отладочным логированием UART
// + защита brownout при старте BT/WiFi
// + байт 1 в MSG_CLIMATE_TELEMETRY = желаемая температура (уставка)

// =============== ОТЛАДКА ===============
#define DEBUG_UART_LOG 0   // 1 — включить вывод запросов/ответов, 0 — выключить

#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "CarData.h"
#include "OBDEngine.h"
#include "DataEngine.h"
#include "TripManager.h"
#include "FuelManager.h"
#include "StorageManager.h"
#include "UARTBinary.h"
#include "Protocol.h"
#include "MPU6050Manager.h"
#include "ThermalController.h"
#include <esp_sleep.h>
#include <time.h>

#define LED_PIN 2
#define PROFILER_PIN 12

extern MPU6050Manager mpuManager;

CarData carData;
OBDEngine obdEngine;
DataEngine dataEngine(obdEngine, carData);
TripManager tripManager(carData, obdEngine);
FuelManager fuelManager(carData);
StorageManager storageManager(carData);
ThermalController thermalController(carData);

const char* WIFI_SSID_PRIMARY   = "Tri-Al(m)";
const char* WIFI_PASS_PRIMARY   = "Ford-Fiona";
const char* WIFI_SSID_SECONDARY = "Tri-AL";
const char* WIFI_PASS_SECONDARY = "Aq1Sw2De3Fr4";
const char* WIFI_SSID_TERTIARY  = "KbKb";
const char* WIFI_PASS_TERTIARY  = "1234567890";
#define WIFI_RSSI_THRESHOLD      (-75)

struct PidStats {
    const char* name;
    const char* cmd;
    uint32_t calls;
    uint32_t totalTime;
    uint32_t minTime;
    uint32_t maxTime;
};

static void runProfiler() {
    Serial.println("[Profiler] Starting PID profiler...");
    delay(2000);
    PidStats stats[7] = {
        {"MAF", "0110", 0, 0, UINT32_MAX, 0},
        {"Speed", "010D", 0, 0, UINT32_MAX, 0},
        {"RPM", "010C", 0, 0, UINT32_MAX, 0},
        {"Coolant Temp", "0105", 0, 0, UINT32_MAX, 0},
        {"Voltage", "ATRV", 0, 0, UINT32_MAX, 0},
        {"Throttle (abs)", "0111", 0, 0, UINT32_MAX, 0},
        {"Throttle (rel)", "0145", 0, 0, UINT32_MAX, 0}
    };
    const int numPids = sizeof(stats) / sizeof(stats[0]);

    if (!obdEngine.isConnected()) {
        Serial.println("[Profiler] Connecting to ELM327...");
        while (!obdEngine.connectWithConfig(carData.btMac, carData.btPin)) {
            Serial.println("[Profiler] Connect failed. Retrying in 5s...");
            delay(5000);
        }
        Serial.println("[Profiler] Connected.");
    }

    uint32_t lastPrint = millis();
    for (;;) {
        for (int i = 0; i < numPids; i++) {
            uint32_t start = millis();
            unsigned long elapsed;
            bool success;
            obdEngine.readPIDWithTime(stats[i].cmd, elapsed, success);
            uint32_t duration = millis() - start;
            if (success) {
                stats[i].calls++;
                stats[i].totalTime += duration;
                if (duration < stats[i].minTime) stats[i].minTime = duration;
                if (duration > stats[i].maxTime) stats[i].maxTime = duration;
            }
            delay(10);
        }
        if (millis() - lastPrint >= 5000) {
            lastPrint = millis();
            Serial.println("=================================");
            Serial.printf("  PID Profiler (uptime %ds)\n", millis()/1000);
            for (int i = 0; i < numPids; i++) {
                if (stats[i].calls > 0) {
                    Serial.printf("%-20s (%-5s) calls: %-5u | min: %3ums | avg: %3ums | max: %3ums\n",
                        stats[i].name, stats[i].cmd, stats[i].calls,
                        stats[i].minTime, stats[i].totalTime / stats[i].calls, stats[i].maxTime);
                } else {
                    Serial.printf("%-20s (%-5s) no responses\n", stats[i].name, stats[i].cmd);
                }
            }
            Serial.println("=================================");
        }
    }
}

extern bool waitingTripAck;
extern bool waitingDayAck;
extern bool waitingDriveAck;
extern uint32_t tripAckStart;
extern uint32_t dayAckStart;
extern uint32_t driveAckStart;
extern void sendTripStats();
extern void sendDayStats();
extern void sendDriveCycles();

volatile bool ntpSyncRequested = false;

bool syncNTP() {
    if (WiFi.status() != WL_CONNECTED) return false;
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    for (int t = 0; t < 10; t++) {
        time_t now = time(nullptr);
        if (now > 1577836800) {
            if (CarData_Lock(100)) {
                carData.systemTime = (uint32_t)now;
                carData.systemSyncTime = (uint32_t)now;
                carData.internetAvailable = true;
                carData.internetDirty = true;
                CarData_Unlock();
            }
            return true;
        }
        delay(500);
    }
    if (CarData_Lock(100)) {
        carData.internetAvailable = false;
        carData.internetDirty = true;
        CarData_Unlock();
    }
    return false;
}

static uint8_t calculateLightLevel(uint32_t unixTime) {
    if (unixTime == 0) return 50;
    uint32_t secondsOfDay = unixTime % 86400;
    int hour = secondsOfDay / 3600;
    int minute = (secondsOfDay % 3600) / 60;
    int totalMinutes = hour * 60 + minute;
    const int start = 8 * 60, peak = 13 * 60, endPeak = 17 * 60, end = 22 * 60;
    if (totalMinutes < start || totalMinutes >= end) return 25;
    else if (totalMinutes < peak) { int elapsed = totalMinutes - start; int range = peak - start; return 25 + (elapsed * 75) / range; }
    else if (totalMinutes < endPeak) return 100;
    else { int elapsed = totalMinutes - endPeak; int range = end - endPeak; return 100 - (elapsed * 75) / range; }
}

// ===================== ОБРАБОТЧИК КОМАНД ОТ ЭКРАНА =====================
void uart_command_handler(uint8_t msg_id, const uint8_t* payload, uint8_t len) {
#if DEBUG_UART_LOG
    Serial.printf("[DBG] CMD: 0x%02X\n", msg_id);
#endif

    switch (msg_id) {
        case MSG_HEARTBEAT_REQ: {
            uint8_t flags = 0;
            uart_send_packet(MSG_HEARTBEAT_RSP, ADDR_ESP32_GW, ADDR_ESP32_P4, &flags, 1);
            break;
        }
        case MSG_RECONNECT: obdEngine.reconnectIfNeeded(); break;

        case MSG_REFUEL_DATA: {
            if (len >= 4) {
                uint16_t liters_x10 = payload[0] | (payload[1] << 8);
                uint16_t price_x10 = payload[2] | (payload[3] << 8);
#if DEBUG_UART_LOG
                Serial.printf("[DBG] REFUEL: %.1f L, price %.1f\n", liters_x10/10.0f, price_x10/10.0f);
#endif
                if (CarData_Lock(100)) {
                    fuelManager.handleRefuel(liters_x10 / 10.0f, false, 0, price_x10 / 10.0f);
                    CarData_Unlock();
                }
            }
            break;
        }
        case MSG_ODO_FULL: {
            if (len >= 4) {
                uint32_t odo = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
#if DEBUG_UART_LOG
                Serial.printf("[DBG] ODO_FULL: %u km\n", odo);
#endif
                if (CarData_Lock(100)) {
                    carData.odoKm = odo; carData.odoDirty = true;
                    CarData_Unlock();
                }
            }
            break;
        }
        case MSG_FULL_TANK_FLAG: {
            if (len >= 1) {
                bool flag = (payload[0] != 0);
                if (CarData_Lock(100)) {
                    carData.hasFirstFullTank = flag;
                    CarData_Unlock();
                }
            }
            break;
        }
        case MSG_SET_FUEL_LEVEL: {
            if (len >= 2) {
                uint16_t level = payload[0] | (payload[1] << 8);
#if DEBUG_UART_LOG
                Serial.printf("[DBG] SET_FUEL_LEVEL: raw=%d -> %d ml\n", level, level*10);
#endif
                if (CarData_Lock(100)) {
                    carData.fuelValueML = level * 10;
                    carData.fuelDirty = true;
                    CarData_Unlock();
                }
            }
            break;
        }
        case MSG_SET_ODO: {
            if (len >= 4) {
                uint32_t odo = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
#if DEBUG_UART_LOG
                Serial.printf("[DBG] SET_ODO: %u km\n", odo);
#endif
                if (CarData_Lock(100)) {
                    carData.odoKm = odo; carData.odoDirty = true;
                    CarData_Unlock();
                }
            }
            break;
        }
        case MSG_REBOOT: ESP.restart(); break;
        case MSG_TRIP_TOGGLE: tripManager.toggleTrip(); break;
        case MSG_WHO_IS_HERE: {
            uint8_t response[2]; response[0] = ADDR_ESP32_GW; response[1] = 0x01;
            uart_send_packet(MSG_I_AM_HERE, ADDR_ESP32_GW, ADDR_ESP32_P4, response, 2);
            break;
        }

        // =================== БЫСТРЫЕ ЗАПРОСЫ ТЕЛЕМЕТРИИ ===================
        case MSG_MAF: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)(carData.mafValue*100.0f); CarData_Unlock(); }
            else { Serial.println("[DBG] MAF lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] MAF val=%.2f g/s -> %d\n", carData.mafValue, v);
#endif
            uart_send_packet(MSG_MAF,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_SPEED: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)carData.speedValue; CarData_Unlock(); }
            else { Serial.println("[DBG] SPEED lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] SPEED val=%d\n", v);
#endif
            uart_send_packet(MSG_SPEED,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_RPM: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)carData.rpmValue; CarData_Unlock(); }
            else { Serial.println("[DBG] RPM lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] RPM val=%d\n", v);
#endif
            uart_send_packet(MSG_RPM,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_COOLANT_TEMP: {
            int16_t v; 
            if (CarData_Lock(10)) { v = (int16_t)carData.tempValue; CarData_Unlock(); }
            else { Serial.println("[DBG] TEMP lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TEMP val=%d\n", v);
#endif
            uart_send_packet(MSG_COOLANT_TEMP,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_VOLTAGE: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)(carData.batValue*100.0f); CarData_Unlock(); }
            else { Serial.println("[DBG] VOLT lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] VOLT val=%.2fV -> %d\n", carData.batValue, v);
#endif
            uart_send_packet(MSG_VOLTAGE,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_OBD_STATUS: {
            uint8_t r; 
            if (CarData_Lock(10)) { r=carData.obdConnected?1:0; CarData_Unlock(); }
            else { Serial.println("[DBG] OBD_STATUS lock failed!"); r = 0; }
#if DEBUG_UART_LOG
            Serial.printf("[DBG] OBD_STATUS=%d\n", r);
#endif
            uart_send_packet(MSG_OBD_STATUS,ADDR_ESP32_GW,ADDR_ESP32_P4,&r,1);
            break;
        }
        case MSG_TRIP_TIME: {
            uint32_t t; 
            if (CarData_Lock(10)) { t = carData.tripValue; CarData_Unlock(); }
            else { Serial.println("[DBG] TRIP_TIME lock failed!"); t = 0; }
            uint8_t r[4]={t&0xFF,(t>>8)&0xFF,(t>>16)&0xFF,(t>>24)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TRIP_TIME=%u\n", t);
#endif
            uart_send_packet(MSG_TRIP_TIME,ADDR_ESP32_GW,ADDR_ESP32_P4,r,4);
            break;
        }
        case MSG_TRIP_PAUSE: {
            uint32_t t; 
            if (CarData_Lock(10)) { t = carData.tripPauseValue; CarData_Unlock(); }
            else { Serial.println("[DBG] TRIP_PAUSE lock failed!"); t = 0; }
            uint8_t r[4]={t&0xFF,(t>>8)&0xFF,(t>>16)&0xFF,(t>>24)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TRIP_PAUSE=%u\n", t);
#endif
            uart_send_packet(MSG_TRIP_PAUSE,ADDR_ESP32_GW,ADDR_ESP32_P4,r,4);
            break;
        }
        case MSG_TRIP_COST: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)(carData.tripFuelUsed * carData.fuelPrice * 100.0f); CarData_Unlock(); }
            else { Serial.println("[DBG] TRIP_COST lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TRIP_COST=%d\n", v);
#endif
            uart_send_packet(MSG_TRIP_COST,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_TRIP_FUEL: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)(carData.tripFuelUsed*100.0f); CarData_Unlock(); }
            else { Serial.println("[DBG] TRIP_FUEL lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TRIP_FUEL=%.2fL -> %d\n", carData.tripFuelUsed, v);
#endif
            uart_send_packet(MSG_TRIP_FUEL,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_FUEL_LEVEL: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)(carData.fuelValueML / 10); CarData_Unlock(); }
            else { Serial.println("[DBG] FUEL_LEVEL lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] FUEL_LEVEL=%d ml (%d)\n", carData.fuelValueML, v);
#endif
            uart_send_packet(MSG_FUEL_LEVEL,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_RANGE: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)carData.rangeValue; CarData_Unlock(); }
            else { Serial.println("[DBG] RANGE lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] RANGE=%d\n", v);
#endif
            uart_send_packet(MSG_RANGE,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_INST_FUEL: {
            uint16_t v; 
            if (CarData_Lock(10)) { v = (uint16_t)(carData.lphValue*100.0f); CarData_Unlock(); }
            else { Serial.println("[DBG] INST_FUEL lock failed!"); v = 0; }
            uint8_t r[2]={v&0xFF,(v>>8)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] INST_FUEL=%.2f L/h -> %d\n", carData.lphValue, v);
#endif
            uart_send_packet(MSG_INST_FUEL,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2);
            break;
        }
        case MSG_ODO: {
            uint32_t odo; 
            if (CarData_Lock(10)) { odo = carData.odoKm; CarData_Unlock(); }
            else { Serial.println("[DBG] ODO lock failed!"); odo = 0; }
            uint8_t r[4]={odo&0xFF,(odo>>8)&0xFF,(odo>>16)&0xFF,(odo>>24)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] ODO=%u\n", odo);
#endif
            uart_send_packet(MSG_ODO,ADDR_ESP32_GW,ADDR_ESP32_P4,r,4);
            break;
        }
        case MSG_TRIP_STATE: {
            uint8_t r; 
            if (CarData_Lock(10)) { r=carData.tripState?1:0; CarData_Unlock(); }
            else { Serial.println("[DBG] TRIP_STATE lock failed!"); r = 0; }
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TRIP_STATE=%d\n", r);
#endif
            uart_send_packet(MSG_TRIP_STATE,ADDR_ESP32_GW,ADDR_ESP32_P4,&r,1);
            break;
        }
        case MSG_TRIP_DIST: {
            uint32_t v; 
            if (CarData_Lock(10)) { v = (uint32_t)(carData.tripDistanceKm*1000.0f); CarData_Unlock(); }
            else { Serial.println("[DBG] TRIP_DIST lock failed!"); v = 0; }
            uint8_t r[4]={v&0xFF,(v>>8)&0xFF,(v>>16)&0xFF,(v>>24)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] TRIP_DIST=%.3f km -> %d m\n", carData.tripDistanceKm, v);
#endif
            uart_send_packet(MSG_TRIP_DIST,ADDR_ESP32_GW,ADDR_ESP32_P4,r,4);
            break;
        }
        case MSG_THROTTLE: {
            uint8_t r; 
            if (CarData_Lock(10)) { r = (uint8_t)(carData.throttlePos+0.5f); CarData_Unlock(); }
            else { Serial.println("[DBG] THROTTLE lock failed!"); r = 0; }
#if DEBUG_UART_LOG
            Serial.printf("[DBG] THROTTLE val=%.1f%% -> %d\n", carData.throttlePos, r);
#endif
            uart_send_packet(MSG_THROTTLE,ADDR_ESP32_GW,ADDR_ESP32_P4,&r,1);
            break;
        }
        case MSG_THROTTLE_REL: {
            uint8_t r; 
            if (CarData_Lock(10)) { r = (uint8_t)(carData.throttleRelPos+0.5f); CarData_Unlock(); }
            else { Serial.println("[DBG] THROTTLE_REL lock failed!"); r = 0; }
#if DEBUG_UART_LOG
            Serial.printf("[DBG] THROTTLE_REL val=%.1f%% -> %d\n", carData.throttleRelPos, r);
#endif
            uart_send_packet(MSG_THROTTLE_REL,ADDR_ESP32_GW,ADDR_ESP32_P4,&r,1);
            break;
        }

        // Освещённость
        case MSG_LIGHT: {
            uint8_t l = DataEngine::getLightLevel(); 
#if DEBUG_UART_LOG
            Serial.printf("[DBG] LIGHT=%d%%\n", l);
#endif
            uart_send_packet(MSG_LIGHT,ADDR_ESP32_GW,ADDR_ESP32_P4,&l,1); break;
        }
        case MSG_LIGHT_SYNTH: {
            uint32_t timeNow;
            if (CarData_Lock(10)) { timeNow = carData.systemTime; CarData_Unlock(); }
            uint8_t l = calculateLightLevel(timeNow); 
#if DEBUG_UART_LOG
            Serial.printf("[DBG] LIGHT_SYNTH=%d%%\n", l);
#endif
            uart_send_packet(MSG_LIGHT_SYNTH,ADDR_ESP32_GW,ADDR_ESP32_P4,&l,1); break;
        }
        case MSG_LIGHT_RAW: {
            uint16_t raw = DataEngine::getLightRaw(); 
#if DEBUG_UART_LOG
            Serial.printf("[DBG] LIGHT_RAW=%d\n", raw);
#endif
            uint8_t r[2]={raw&0xFF,(raw>>8)&0xFF}; uart_send_packet(MSG_LIGHT_RAW,ADDR_ESP32_GW,ADDR_ESP32_P4,r,2); break;
        }
        case MSG_REQ_TIME: {
            uint32_t t; 
            if (CarData_Lock(10)) { t = carData.systemTime; CarData_Unlock(); }
            uint8_t r[4]={t&0xFF,(t>>8)&0xFF,(t>>16)&0xFF,(t>>24)&0xFF};
#if DEBUG_UART_LOG
            Serial.printf("[DBG] REQ_TIME=%u\n", t);
#endif
            uart_send_packet(MSG_TIME,ADDR_ESP32_GW,ADDR_ESP32_P4,r,4);
            break;
        }
        case MSG_INTERNET_SYNC: {
            ntpSyncRequested = true;
            uint8_t r;
            if (CarData_Lock(10)) { r = carData.internetAvailable ? 1 : 0; CarData_Unlock(); }
            else r = 0;
#if DEBUG_UART_LOG
            Serial.printf("[DBG] INTERNET_SYNC resp=%d\n", r);
#endif
            uart_send_packet(MSG_INTERNET_SYNC,ADDR_ESP32_GW,ADDR_ESP32_P4,&r,1);
            break;
        }

        // Статистика
        case MSG_REQ_TRIP_STATS: { sendTripStats(); waitingTripAck = true; tripAckStart = millis(); break; }
        case MSG_REQ_DAY_STATS: { sendDayStats(); waitingDayAck = true; dayAckStart = millis(); break; }
        case MSG_TRIP_STATS_ACK:
            if (waitingTripAck) {
                waitingTripAck = false;
                if (CarData_Lock(100)) {
                    carData.tripStatPending = false;
                    if (!obdEngine.isEmulationMode()) storageManager.requestStateSave();
                    CarData_Unlock();
                }
            }
            break;
        case MSG_DAY_STATS_ACK:
            if (waitingDayAck) {
                waitingDayAck = false;
                if (CarData_Lock(100)) {
                    carData.dayStatPending = false;
                    if (!obdEngine.isEmulationMode()) storageManager.requestStateSave();
                    CarData_Unlock();
                }
            }
            break;
        case MSG_REQ_DRIVE_CYCLES: { sendDriveCycles(); waitingDriveAck = true; driveAckStart = millis(); break; }
        case MSG_DRIVE_CYCLES_ACK:
            if (waitingDriveAck) {
                waitingDriveAck = false;
                tripManager.clearDriveCycles();
                if (!obdEngine.isEmulationMode()) {
                    if (CarData_Lock(100)) {
                        storageManager.requestStateSave();
                        CarData_Unlock();
                    }
                }
            }
            break;
        case MSG_REQ_API: { uint8_t d=0; uart_send_packet(MSG_API_RESP,ADDR_ESP32_GW,ADDR_ESP32_P4,&d,1); break; }

        // Акселерометр MPU-6050
        case MSG_REQ_ACCEL: {
            int16_t ax = (int16_t)(mpuManager.getAccelX() * 100.0f);
            int16_t ay = (int16_t)(mpuManager.getAccelY() * 100.0f);
            int16_t az = (int16_t)(mpuManager.getAccelZ() * 100.0f);
            uint8_t resp1[4] = {ax & 0xFF, (ax >> 8) & 0xFF, ay & 0xFF, (ay >> 8) & 0xFF};
            uart_send_packet(MSG_REQ_ACCEL, ADDR_ESP32_GW, ADDR_ESP32_P4, resp1, 4);
            uint8_t resp2[4] = {az & 0xFF, (az >> 8) & 0xFF, 0, 0};
            uart_send_packet(MSG_ACCEL_Z, ADDR_ESP32_GW, ADDR_ESP32_P4, resp2, 4);
            break;
        }
        case MSG_REQ_GYRO: {
            int16_t gx = (int16_t)(mpuManager.getGyroX() * 100.0f);
            int16_t gy = (int16_t)(mpuManager.getGyroY() * 100.0f);
            int16_t gz = (int16_t)(mpuManager.getGyroZ() * 100.0f);
            uint8_t resp[6] = {gx & 0xFF, (gx >> 8) & 0xFF, gy & 0xFF, (gy >> 8) & 0xFF, gz & 0xFF, (gz >> 8) & 0xFF};
            uart_send_packet(MSG_REQ_GYRO, ADDR_ESP32_GW, ADDR_ESP32_P4, resp, 6);
            break;
        }
        case MSG_REQ_TILT: {
            int8_t roll = (int8_t)(mpuManager.getRoll());
            int8_t pitch = (int8_t)(mpuManager.getPitch());
            uint8_t resp[2] = {roll, pitch};
            uart_send_packet(MSG_REQ_TILT, ADDR_ESP32_GW, ADDR_ESP32_P4, resp, 2);
            break;
        }
        case MSG_CALIBRATE_ACCEL: { mpuManager.calibrateAlignment(); break; }
        case MSG_REQ_CALIB_STATUS: {
            uint8_t status = mpuManager.getCalibStatus();
            uart_send_packet(MSG_REQ_CALIB_STATUS, ADDR_ESP32_GW, ADDR_ESP32_P4, &status, 1);
            break;
        }

        // Климат-контроль
        case MSG_FAN_SET_MODE:         if (len>=1) thermalController.setFanMode(payload[0]); break;
        case MSG_FAN_SET_PWM1:         if (len>=1) thermalController.setFanPWM1(payload[0]); break;
        case MSG_FAN_SET_PWM2:         if (len>=1) thermalController.setFanPWM2(payload[0]); break;
        case MSG_FAN_SET_AUTO:         if (len>=1) thermalController.setFanAuto(payload[0] != 0); break;
        case MSG_CLIMATE_SET_PRESET:   if (len>=1) thermalController.setClimatePreset(payload[0]); break;
        case MSG_CLIMATE_SET_TEMP:     if (len>=2) { int16_t t = payload[0]|(payload[1]<<8); thermalController.setClimateSetpoint(t/10.0f); } break;
        case MSG_CLIMATE_SET_PWM:      if (len>=1) thermalController.setClimatePWM(payload[0]); break;
        case MSG_CLIMATE_SET_AUTO:     if (len>=1) thermalController.setClimateAuto(payload[0] != 0); break;
        case MSG_FAN_TELEMETRY: {
            int16_t temp = (int16_t)(thermalController.getCoolantTemp() * 10.0f);
            uint8_t resp[6] = {
                thermalController.getFanPWM1(),
                thermalController.getFanPWM2(),
                thermalController.getFanMode(),
                thermalController.getFanAuto() ? 1 : 0,
                (uint8_t)(temp & 0xFF),
                (uint8_t)((temp >> 8) & 0xFF)
            };
            uart_send_packet(MSG_FAN_TELEMETRY, ADDR_ESP32_GW, ADDR_ESP32_P4, resp, 6);
            break;
        }
        case MSG_CLIMATE_TELEMETRY: {
            int16_t temp = (int16_t)(thermalController.getCabinTemp() * 10.0f);
            // Байт 1 теперь содержит желаемую температуру (уставку) вместо пресета
            uint8_t setpointByte = (uint8_t)(thermalController.getClimateSetpoint() + 0.5f);
            uint8_t resp[6] = {
                thermalController.getClimatePWM(),
                setpointByte,                                      // желаемая температура
                thermalController.getClimateAuto() ? 1 : 0,
                (uint8_t)(temp & 0xFF),
                (uint8_t)((temp >> 8) & 0xFF),
                0
            };
            uart_send_packet(MSG_CLIMATE_TELEMETRY, ADDR_ESP32_GW, ADDR_ESP32_P4, resp, 6);
            break;
        }

        // Калибровка вентиляторов радиатора
        case MSG_FAN_CALIB_START:       if (len>=1) thermalController.startFanCalib(payload[0]); break;
        case MSG_FAN_CALIB_STEP:        if (len>=1) thermalController.stepFanCalib((int8_t)payload[0]); break;
        case MSG_FAN_CALIB_START_POINT: thermalController.saveFanCalibPoint(0); break;
        case MSG_FAN_CALIB_STOP_POINT:  thermalController.saveFanCalibPoint(1); break;
        case MSG_FAN_CALIB_NOISE_LOW:   thermalController.saveFanCalibPoint(2); break;
        case MSG_FAN_CALIB_NOISE_HIGH:  thermalController.saveFanCalibPoint(3); break;
        case MSG_FAN_CALIB_SAVE:        thermalController.saveFanCalib(); break;

        // Калибровка печки
        case MSG_HEATER_CALIB_START:        thermalController.startHeaterCalib(); break;
        case MSG_HEATER_CALIB_STEP:         if (len>=1) thermalController.stepHeaterCalib((int8_t)payload[0]); break;
        case MSG_HEATER_CALIB_START_POINT:  thermalController.saveHeaterCalibPoint(0); break;
        case MSG_HEATER_CALIB_STOP_POINT:   thermalController.saveHeaterCalibPoint(1); break;
        case MSG_HEATER_CALIB_NOISE_LOW:    thermalController.saveHeaterCalibPoint(2); break;
        case MSG_HEATER_CALIB_NOISE_HIGH:   thermalController.saveHeaterCalibPoint(3); break;
        case MSG_HEATER_CALIB_SAVE:         thermalController.saveHeaterCalib(); break;

        default: break;
    }
}

unsigned long getUnixTime() {
    time_t now;
    time(&now);
    if (now > 1577836800) return (unsigned long)now;
    return 0;
}

void enterLightSleep() {
    Serial.println("Entering light sleep for 5 minutes...");
    esp_sleep_enable_timer_wakeup(5 * 60 * 1000000ULL);
    esp_light_sleep_start();
    Serial.println("Woke up from light sleep");
}

void dataTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50);
    uint32_t lastLedToggle = 0;
    static uint32_t lastActivityTime = millis();
    static uint32_t lastTimeUpdate = 0;
    static uint32_t lastThermalTime = 0;

    uint32_t lastLedBlinkTime = 0;
    uint8_t ledMode = 0;
    uint16_t ledBlinkPeriod = 2000;
    uint16_t ledBlinkDuration = 250;

    for (;;) {
        dataEngine.process();

        uint32_t now = millis();

        if (now - lastTimeUpdate >= 1000) {
            lastTimeUpdate = now;
            if (CarData_Lock(10)) {
                if (carData.systemTime > 1577836800) {
                    carData.systemTime = (uint32_t)time(nullptr);
                }
                CarData_Unlock();
            }
        }

        DataEngine::updateLight();
        mpuManager.update();
        mpuManager.processCalibration();

        if (now - lastThermalTime >= 500) {
            lastThermalTime = now;
            thermalController.process();
        }

        tripManager.process();
        fuelManager.process();
        storageManager.process();
        obdEngine.reconnectIfNeeded();

        bool obdOk = obdEngine.isEmulationMode() || obdEngine.isConnected();
        bool internetOk = false;
        if (CarData_Lock(10)) {
            internetOk = carData.internetAvailable;
            CarData_Unlock();
        }

        if (!obdOk) {
            ledMode = 2;
            ledBlinkPeriod = 200;
            ledBlinkDuration = 100;
        } else if (!internetOk) {
            ledMode = 1;
            ledBlinkPeriod = 2000;
            ledBlinkDuration = 100;
        } else {
            ledMode = 0;
            ledBlinkPeriod = 2000;
            ledBlinkDuration = 250;
        }

        if (obdEngine.isEmulationMode()) {
            ledMode = 3;
            ledBlinkPeriod = 200;
            ledBlinkDuration = 100;
        }

        {
            uint32_t cycleTime = now - lastLedBlinkTime;
            if (cycleTime >= ledBlinkPeriod) {
                lastLedBlinkTime = now;
                cycleTime = 0;
            }
            if (cycleTime < ledBlinkDuration) {
                digitalWrite(LED_PIN, HIGH);
            } else {
                digitalWrite(LED_PIN, LOW);
            }
        }

        if (CarData_Lock(10)) {
            if (carData.rpmValue < 300 && !carData.tripState && !obdEngine.isConnected() && !obdEngine.isEmulationMode()) {
                if (now - lastActivityTime > (carData.sleepTimeout * 60000UL)) {
                    CarData_Unlock();
                    enterLightSleep();
                    lastActivityTime = millis();
                } else {
                    CarData_Unlock();
                }
            } else {
                lastActivityTime = millis();
                CarData_Unlock();
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void uartTask(void *pvParameters) {
    for (;;) {
        uart_task_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void wifiTask(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    for (;;) {
        if (ntpSyncRequested) {
            ntpSyncRequested = false;
            syncNTP();
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Not connected, trying...");
            WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
            const char* ssids[] = {WIFI_SSID_PRIMARY, WIFI_SSID_SECONDARY, WIFI_SSID_TERTIARY};
            const char* passes[] = {WIFI_PASS_PRIMARY, WIFI_PASS_SECONDARY, WIFI_PASS_TERTIARY};
            for (int i = 0; i < 3; i++) {
                WiFi.begin(ssids[i], passes[i]);
                for (int j = 0; j < 20; j++) {
                    if (WiFi.status() == WL_CONNECTED) break;
                    delay(500);
                }
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.printf("[WiFi] Connected to %s\n", ssids[i]);
                    delay(1000);
                    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
                    syncNTP();
                    break;
                }
            }
            if (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
                Serial.println("[WiFi] All networks failed, retrying in 10s...");
                vTaskDelay(pdMS_TO_TICKS(10000));
                continue;
            }
        }

        int8_t rssi = WiFi.RSSI();
        String ssid = WiFi.SSID();

        if (CarData_Lock(100)) {
            carData.wifiRSSI = rssi;

            if (ssid != WIFI_SSID_PRIMARY && rssi < WIFI_RSSI_THRESHOLD) {
                Serial.printf("[WiFi] Low signal (%d dBm) on %s, switching to %s\n", rssi, ssid.c_str(), WIFI_SSID_PRIMARY);
                WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
                WiFi.disconnect(false);
                WiFi.begin(WIFI_SSID_PRIMARY, WIFI_PASS_PRIMARY);
                for (int j = 0; j < 20; j++) {
                    if (WiFi.status() == WL_CONNECTED) break;
                    delay(500);
                }
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.printf("[WiFi] Switched to %s\n", WIFI_SSID_PRIMARY);
                    syncNTP();
                } else {
                    Serial.println("[WiFi] Switch failed, will try again next cycle");
                }
                delay(1000);
                WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
            }

            if (!carData.internetAvailable && WiFi.status() == WL_CONNECTED) {
                CarData_Unlock();
                syncNTP();
            } else {
                carData.wifiConnected = (WiFi.status() == WL_CONNECTED);
                CarData_Unlock();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32 OBD Gateway Starting ===");

    CarData_init(&carData);

    pinMode(PROFILER_PIN, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);

    bool profilerMode = (digitalRead(PROFILER_PIN) == LOW);
    bool debugMode = (digitalRead(13) == LOW);

    if (profilerMode && !debugMode) {
        Serial.println("D12 LOW, D13 HIGH - Entering PROFILER mode");
        runProfiler();
    }

    if (debugMode) {
        Serial.println("GPIO13 LOW - OBD emulation mode.");
        obdEngine.setEmulationMode(true);
    } else {
        Serial.println("GPIO13 HIGH - normal OBD mode.");
        obdEngine.setEmulationMode(false);
    }

    Serial.println("\n--- OBD Engine initialization ---");
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    bool obdBeginOk = obdEngine.begin();
    if (!obdBeginOk) {
        Serial.println("OBD Engine begin failed");
        delay(1000);
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
    } else {
        if (!obdEngine.isEmulationMode()) {
            int attempt = 0;
            bool connected = false;
            while (!connected && attempt <= 5) {
                connected = obdEngine.connectWithConfig(carData.btMac, carData.btPin);
                if (!connected) {
                    Serial.println("OBD connect failed, retrying in 5s...");
                    delay(5000);
                    attempt++;
                }
            }
            if (connected) {
                Serial.println("OBD Engine connected");
                CarData_Lock(100);
                carData.obdConnected = true;
                CarData_Unlock();
            } else {
                Serial.println("Giving up, will retry in background.");
                CarData_Lock(100);
                carData.obdConnected = false;
                CarData_Unlock();
            }
        } else {
            Serial.println("OBD emulation mode, no connection needed.");
            CarData_Lock(100);
            carData.obdConnected = true;
            CarData_Unlock();
        }
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
    }

    if (!mpuManager.begin()) {
        Serial.println("[MPU] Initialization failed");
    } else {
        Serial.printf("[MPU] Status: %d\n", mpuManager.getCalibStatus());
    }

    thermalController.begin();

    if (storageManager.begin()) {
        storageManager.loadConfig();
        storageManager.loadState();
    }

    Serial.println("Performing initial PID queries...");
    if (carData.obdConnected) {
        bool success; unsigned long elapsed; float val;
        val = obdEngine.readPIDWithTime("0110", elapsed, success);
        if (success) DataEngine::updateMAF(&carData, val);
        val = obdEngine.readPIDWithTime("010D", elapsed, success);
        if (success) DataEngine::updateSpeed(&carData, val);
        val = obdEngine.readPIDWithTime("010C", elapsed, success);
        if (success) DataEngine::updateRPM(&carData, val);
        val = obdEngine.readPIDWithTime("0105", elapsed, success);
        if (success) DataEngine::updateCoolantTemp(&carData, val);
        val = obdEngine.readPIDWithTime("ATRV", elapsed, success);
        if (success) DataEngine::updateVoltage(&carData, val);
    }

    tripManager.setTimeProvider(getUnixTime);
    dataEngine.begin();
    tripManager.begin();
    fuelManager.begin();

    uart_binary_init(921600, 16, 17);
    uart_set_command_callback(uart_command_handler);
    Serial.printf("[UART] Initialized on pins 16,17 at %d baud\n", 921600);

    xTaskCreatePinnedToCore(dataTask, "DataTask", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(uartTask,  "UartTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(wifiTask,  "WifiTask", 4096, NULL, 1, NULL, 1);
    Serial.println("Setup complete. DataTask on core 0, UartTask+WifiTask on core 1.\n");
}

void loop() {
    vTaskDelay(1000);
}