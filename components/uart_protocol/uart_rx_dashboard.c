/**
 * @file uart_rx_dashboard.c
 * @brief Обработка пакетов с дашборд-данными (скорость, обороты, температура и т.д.)
 */

#include "uart_rx_dashboard.h"
#include "protocol.h"
#include "CarData.h"
#include <sys/time.h>

void uart_rx_dashboard_process(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(10);

    switch (msg_id) {
        case MSG_MAF:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->mafValue = val;
                data->mafDirty = true;
            }
            break;
        case MSG_SPEED:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->speedValue = val;
                data->speedDirty = true;
            }
            break;
        case MSG_RPM:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->rpmValue = val;
                data->rpmDirty = true;
            }
            break;
        case MSG_COOLANT_TEMP:
            if (len >= 2) {
                int16_t val = (int16_t)((payload[1] << 8) | payload[0]);
                data->tempValue = val;
                data->tempDirty = true;
            }
            break;
        case MSG_VOLTAGE:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->batValue = val / 100.0f;
                data->batDirty = true;
            }
            break;
        case MSG_OBD_STATUS:
            if (len >= 1) {
                data->obdConnected = payload[0] != 0;
            }
            break;
        case MSG_TRIP_TIME:
            if (len >= 4) {
                uint32_t val = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
                data->tripValue = val;
                data->tripTimeDirty = true;
            }
            break;
        case MSG_TRIP_PAUSE:
            if (len >= 4) {
                uint32_t val = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
                data->tripPauseValue = val;
                data->tripPauseDirty = true;
            }
            break;
        case MSG_TRIP_COST:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->tripMValue = val / 100.0f;
                data->tripCostDirty = true;
            }
            break;
        case MSG_TRIP_FUEL:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->tripFuelUsed = val / 100.0f;
                data->tripFuelDirty = true;
            }
            break;
        case MSG_FUEL_LEVEL:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->fuelValue = val / 100.0f;
                data->fuelDirty = true;
            }
            break;
        case MSG_RANGE:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->rangeValue = val;
                data->rangeDirty = true;
            }
            break;
        case MSG_INST_FUEL:
            if (len >= 2) {
                uint16_t val = (payload[1] << 8) | payload[0];
                data->lphValue = val / 100.0f;
                data->lphDirty = true;
            }
            break;
        case MSG_ODO:
            if (len >= 4) {
                uint32_t val = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
                data->odoKm = val;
                data->odoDirty = true;
            }
            break;
        case MSG_TRIP_STATUS:
            if (len >= 1) {
                data->tripState = (payload[0] != 0);
            }
            break;
        case MSG_TRIP_DIST:
            if (len >= 4) {
                uint32_t meters = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
                data->tripDistanceKm = meters / 1000.0f;
                data->tripDistDirty = true;
            }
            break;
        case MSG_INTERNET_SYNC:
            if (len >= 1) {
                data->internetAvailable = (payload[0] == 1);
                data->internetDirty = true;
            }
            break;
        case MSG_TIME:
            if (len >= 4) {
                uint32_t time = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
                if (time != 0) {
                    if (!data->time_set_manually) {
                        data->systemTime = time;
                        struct timeval tv;
                        tv.tv_sec = time;
                        tv.tv_usec = 0;
                        settimeofday(&tv, NULL);
                        data->time_synced = true;
                        data->time_received_this_boot = true;
                    }
                }
            }
            break;
        case MSG_API_RESP:
            break;
        case MSG_LIGHT:
            if (len >= 1) {
                data->ambient_light_pct = payload[0];
            }
            break;
        case MSG_LIGHT_SYNTH:
            if (len >= 1) {
                data->ambient_light_synth = payload[0];
            }
            break;
        case MSG_LIGHT_RAW:
            if (len >= 2) {
                uint16_t raw = payload[0] | (payload[1] << 8);
                data->ambient_light_raw = raw;
            }
            break;
        case MSG_THROTTLE:
            if (len >= 1) {
                data->throttlePos = (float)payload[0];
                data->throttleDirty = true;
            }
            break;
        case MSG_THROTTLE_REL:
            if (len >= 1) {
                data->throttlePos = (float)payload[0];
                data->throttleDirty = true;
            }
            break;
        default:
            break;
    }
    CarData_Unlock();
}