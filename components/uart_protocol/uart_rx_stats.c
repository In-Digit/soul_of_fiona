/**
 * @file uart_rx_stats.c
 * @brief Обработка пакетов статистики (поездки, сутки, заезды)
 */

#include "uart_rx_stats.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_STATS";

bool s_in_drive_cycle = false;
static uint8_t s_drive_cycle_total = 0;
static uint8_t s_drive_cycle_current = 0;
static uint8_t s_drive_cycle_field_index = 0;

static void handle_trip_stats_packet(uint8_t msg_id, const uint8_t *payload, uint8_t len);
static void handle_day_stats_packet(uint8_t msg_id, const uint8_t *payload, uint8_t len);
static void handle_drive_cycle_packet(uint8_t msg_id, const uint8_t *payload, uint8_t len);

void uart_rx_stats_process(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    // Статистика поездок
    if ((msg_id >= 0x35 && msg_id <= 0x3F) || (msg_id >= 0x55 && msg_id <= 0x5A)) {
        handle_trip_stats_packet(msg_id, payload, len);
        return;
    }

    // Суточная статистика
    if ((msg_id >= 0x42 && msg_id <= 0x4A) || (msg_id >= 0x5B && msg_id <= 0x64)) {
        handle_day_stats_packet(msg_id, payload, len);
        return;
    }

    // Заезды (Drive Cycles)
    if (msg_id == MSG_DRIVE_CYCLE_DATA || s_in_drive_cycle ||
        msg_id == MSG_DRIVE_CYCLE_END || (msg_id >= 0x50 && msg_id <= 0x5C && s_in_drive_cycle)) {
        handle_drive_cycle_packet(msg_id, payload, len);
        return;
    }
}

static void handle_trip_stats_packet(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(10);

    if (msg_id == MSG_TRIP_STATS_START) {
        if (len >= 1) {
            data->trip_rx.expected_count = payload[0];
            data->trip_rx.received_count = 0;
            data->trip_rx.active = true;
            memset(&data->trip_rx.data, 0, sizeof(TripStats));
        }
    } else if (data->trip_rx.active) {
        switch (msg_id) {
            case MSG_TRIP_STAT_START_TIME: if(len>=4) data->trip_rx.data.start_time = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_TRIP_STAT_STATUS:     if(len>=1) data->trip_rx.data.status = payload[0]; break;
            case MSG_TRIP_STAT_DURATION:   if(len>=4) data->trip_rx.data.duration_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_TRIP_STAT_PAUSE_TIME: if(len>=4) data->trip_rx.data.pause_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_TRIP_STAT_PAUSE_CNT:  if(len>=2) data->trip_rx.data.pause_count = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_DIST:       if(len>=4) data->trip_rx.data.distance_m = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_TRIP_STAT_FUEL:       if(len>=2) data->trip_rx.data.fuel_x100 = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_MAX_SPEED:  if(len>=2) data->trip_rx.data.max_speed = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_MAX_LPH:    if(len>=2) data->trip_rx.data.max_lph_x100 = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_WARMUP:            if(len>=4) data->trip_rx.data.warmup_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_TRIP_STAT_AVG_THROTTLE_REL:  if(len>=2) data->trip_rx.data.avg_throttle_rel_x100 = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_MAX_THROTTLE_REL:  if(len>=2) data->trip_rx.data.max_throttle_rel = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_AGGRESSIVE_COUNT:  if(len>=2) data->trip_rx.data.aggressive_count = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_FULL_THROTTLE_CNT: if(len>=2) data->trip_rx.data.full_throttle_count = payload[0]|(payload[1]<<8); break;
            case MSG_TRIP_STAT_MOVING_TIME:       if(len>=4) data->trip_rx.data.moving_time_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            default: break;
        }
        data->trip_rx.received_count++;
    } else if (msg_id == MSG_TRIP_STATS_END && data->trip_rx.active) {
        if (data->trip_rx.received_count >= data->trip_rx.expected_count) {
            data->trip_rx.data.valid = true;
            memcpy(&data->lastTripStats, &data->trip_rx.data, sizeof(TripStats));
            ESP_LOGI(TAG, "Trip stats received");
        }
        data->trip_rx.active = false;
        data->trip_stats_pending = false;
        uart_send_to_gateway(MSG_TRIP_STATS_ACK, NULL, 0);
    }
    CarData_Unlock();
}

static void handle_day_stats_packet(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(10);

    if (msg_id == MSG_DAY_STATS_START) {
        if (len >= 1) {
            data->day_rx.expected_count = payload[0];
            data->day_rx.received_count = 0;
            data->day_rx.active = true;
            memset(&data->day_rx.data, 0, sizeof(DayStats));
        }
    } else if (data->day_rx.active) {
        switch (msg_id) {
            case MSG_DAY_STAT_DATE:      if(len>=4) data->day_rx.data.date = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_DAY_STAT_VALID:     if(len>=1) data->day_rx.data.valid_flag = payload[0]; break;
            case MSG_DAY_STAT_ENG_SEC:   if(len>=4) data->day_rx.data.engine_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_DAY_STAT_DIST:      if(len>=4) data->day_rx.data.distance_m = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_DAY_STAT_FUEL:      if(len>=2) data->day_rx.data.fuel_x100 = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_MAX_SPEED: if(len>=2) data->day_rx.data.max_speed = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_MAX_LPH:   if(len>=2) data->day_rx.data.max_lph_x100 = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_FIRST_START:        if(len>=4) data->day_rx.data.first_start_time = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_DAY_STAT_LAST_STOP:          if(len>=4) data->day_rx.data.last_stop_time = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_DAY_STAT_TRIP_COUNT:         if(len>=1) data->day_rx.data.trip_count = payload[0]; break;
            case MSG_DAY_STAT_DRIVE_COUNT:        if(len>=1) data->day_rx.data.drive_count = payload[0]; break;
            case MSG_DAY_STAT_AVG_THROTTLE_REL:   if(len>=2) data->day_rx.data.avg_throttle_rel_x100 = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_MAX_THROTTLE_REL:   if(len>=2) data->day_rx.data.max_throttle_rel = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_WARMUP:             if(len>=4) data->day_rx.data.warmup_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            case MSG_DAY_STAT_AGGRESSIVE_COUNT:   if(len>=2) data->day_rx.data.aggressive_count = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_FULL_THROTTLE_CNT:  if(len>=2) data->day_rx.data.full_throttle_count = payload[0]|(payload[1]<<8); break;
            case MSG_DAY_STAT_MOVING_TIME:        if(len>=4) data->day_rx.data.moving_time_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
            default: break;
        }
        data->day_rx.received_count++;
    } else if (msg_id == MSG_DAY_STATS_END && data->day_rx.active) {
        if (data->day_rx.received_count >= data->day_rx.expected_count) {
            data->day_rx.data.valid = true;
            memcpy(&data->lastDayStats, &data->day_rx.data, sizeof(DayStats));
            ESP_LOGI(TAG, "Day stats received");
        }
        data->day_rx.active = false;
        data->day_stats_pending = false;
        uart_send_to_gateway(MSG_DAY_STATS_ACK, NULL, 0);
    }
    CarData_Unlock();
}

static void handle_drive_cycle_packet(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(10);

    if (msg_id == MSG_DRIVE_CYCLE_DATA && !s_in_drive_cycle) {
        if (len >= 1) {
            s_drive_cycle_total = payload[0];
            s_drive_cycle_current = 0;
            s_drive_cycle_field_index = 0;
            s_in_drive_cycle = true;
            data->receivedDriveCycleCount = 0;
            ESP_LOGI(TAG, "Receiving %d drive cycles", s_drive_cycle_total);
        }
    } else if (s_in_drive_cycle) {
        if (msg_id == MSG_DRIVE_CYCLE_END) {
            s_in_drive_cycle = false;
            data->receivedDriveCycleCount = s_drive_cycle_total;
            ESP_LOGI(TAG, "All drive cycles received, sending ACK");
            uart_send_to_gateway(MSG_DRIVE_CYCLES_ACK, NULL, 0);
        } else if (s_drive_cycle_current < s_drive_cycle_total) {
            DriveCycle *dc = &data->receivedDriveCycles[s_drive_cycle_current];
            uint8_t field = msg_id - 0x50;
            if (field <= 12) {
                switch (field) {
                    case 0: if(len>=4) dc->start_time = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
                    case 1: if(len>=4) dc->end_time   = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
                    case 2: if(len>=4) dc->duration_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
                    case 3: if(len>=4) dc->distance_m = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
                    case 4: if(len>=2) dc->fuel_x100  = payload[0]|(payload[1]<<8); break;
                    case 5: if(len>=2) dc->max_speed  = payload[0]|(payload[1]<<8); break;
                    case 6: if(len>=2) dc->max_lph_x100 = payload[0]|(payload[1]<<8); break;
                    case 7: if(len>=2) dc->avg_throttle_rel_x100 = payload[0]|(payload[1]<<8); break;
                    case 8: if(len>=2) dc->max_throttle_rel = payload[0]|(payload[1]<<8); break;
                    case 9: if(len>=2) dc->aggressive_count = payload[0]|(payload[1]<<8); break;
                    case 10: if(len>=2) dc->full_throttle_count = payload[0]|(payload[1]<<8); break;
                    case 11: if(len>=4) dc->warmup_sec = payload[0]|(payload[1]<<8)|(payload[2]<<16)|(payload[3]<<24); break;
                    default: break;
                }
                s_drive_cycle_field_index++;
                if (s_drive_cycle_field_index >= 12) {
                    dc->valid = true;
                    s_drive_cycle_current++;
                    s_drive_cycle_field_index = 0;
                }
            }
        }
    }
    CarData_Unlock();
}