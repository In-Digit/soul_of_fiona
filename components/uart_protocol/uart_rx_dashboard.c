/**
 * @file uart_rx_dashboard.c
 * @brief Обработка пакетов с дашборд-данными (скорость, обороты, температура, климат и т.д.)
 *        Исправлен парсинг MSG_CLIMATE_TELEMETRY согласно протоколу шлюза от 2026-06-11.
 */

#include "uart_rx_dashboard.h"
#include "protocol.h"
#include "CarData.h"
#include "fiona_brain.h"
#include "rx8025_rtc.h"
#include <sys/time.h>
#include <time.h>

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

                        // Сохраняем время во внешний RTC
                        time_t utc_time = time;
                        struct tm *lt = localtime(&utc_time);
                        if (lt) {
                            rtc_time_t rtc_time;
                            rtc_time.year = lt->tm_year + 1900;
                            rtc_time.mon  = lt->tm_mon + 1;
                            rtc_time.day  = lt->tm_mday;
                            rtc_time.hour = lt->tm_hour;
                            rtc_time.min  = lt->tm_min;
                            rtc_time.sec  = lt->tm_sec;
                            rtc_set_time(&rtc_time);
                        }
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

        // ============== ИСПРАВЛЕННЫЙ ПАРСИНГ КЛИМАТ-ТЕЛЕМЕТРИИ ==============
        case MSG_CLIMATE_TELEMETRY:   // 0xC9
            if (len >= 6) {
                // Протокол шлюза (2026-06-11):
                // payload[0] = heater_pwm (uint8_t)
                // payload[1] = target_temp (uint8_t, целые градусы)
                // payload[2] = auto_flag (uint8_t)
                // payload[3] = cabin_temp_lo (младший байт int16_t)
                // payload[4] = cabin_temp_hi (старший байт int16_t)
                // payload[5] = reserved
                data->heater_pwm = payload[0];
                data->climate_target_temp = (float)payload[1];   // Целое число градусов
                // auto_flag можно сохранить в будущем при необходимости
                int16_t cabin_raw = (int16_t)(payload[3] | (payload[4] << 8));
                data->cabin_temp = cabin_raw / 10.0f;
                data->heater_pwm_dirty = true;
                data->climate_target_dirty = true;
                data->cabin_temp_dirty = true;
            }
            break;

        // --------------- СТИЛЬ ВОЖДЕНИЯ ОТ ШЛЮЗА (0xE2) ---------------
        case MSG_REQ_DRIVING_STYLE:
            if (len >= 1) {
                uint8_t style = payload[0];
                FionaState *state = fiona_brain_get_state();
                if (state && state->manual_style == 0) {
                    state->driving_style = style;
                }
            }
            break;

        default:
            break;
    }
    CarData_Unlock();
}