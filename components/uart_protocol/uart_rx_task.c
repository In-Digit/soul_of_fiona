/**
 * @file uart_rx_task.c
 * @brief Задача приёма UART-пакетов и диспетчеризация по обработчикам.
 *        Классический приём кадров фиксированной длины (10 байт).
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "uart_protocol.h"
#include "uart_rx_task.h"
#include "uart_rx_dashboard.h"
#include "uart_rx_stats.h"
#include "uart_rx_arduino.h"
#include "uart_rx_imu.h"
#include "CarData.h"
#include <string.h>
#include <sys/time.h>

static const char *TAG = "UART_RX";

extern volatile int64_t last_rx_time_us;
extern volatile uint8_t last_rx_msg_id;

void uart_rx_task(void *arg) {
    size_t port_count = (size_t)arg;
    uint8_t data[128];
    uint8_t rx_buffer[FRAME_TOTAL_SIZE];
    uint8_t rx_index = 0;
    bool in_frame = false;

    while (1) {
        for (size_t i = 0; i < port_count; i++) {
            int uart_num = (i == 0) ? UART_NUM_1 : UART_NUM_2;
            int len = uart_read_bytes(uart_num, data, sizeof(data), 0);
            for (int j = 0; j < len; j++) {
                uint8_t b = data[j];
                if (!in_frame) {
                    if (b == FRAME_MAGIC) {
                        rx_index = 0;
                        rx_buffer[rx_index++] = b;
                        in_frame = true;
                    }
                } else {
                    rx_buffer[rx_index++] = b;
                    if (rx_index >= FRAME_TOTAL_SIZE) {
                        uint8_t crc_calc = crc8_calculate(rx_buffer, FRAME_TOTAL_SIZE - 1);
                        if (crc_calc == rx_buffer[FRAME_TOTAL_SIZE - 1]) {
                            process_frame(uart_num, rx_buffer);
                            last_rx_time_us = esp_timer_get_time();
                        } else {
                            ESP_LOGW(TAG, "CRC error on UART%d", uart_num);
                        }
                        in_frame = false;
                        rx_index = 0;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void process_frame(int uart_num, const uint8_t *frame) {
    uint8_t msg_id = frame[1];
    uint8_t src    = frame[2];
    uint8_t dst    = frame[3];
    uint8_t len    = frame[4];
    const uint8_t *payload = &frame[5];

    last_rx_msg_id = msg_id;

    // Heartbeat запрос от шлюза (теперь с временем)
    if (msg_id == MSG_HEARTBEAT_REQ) {
        if (len >= 4) {
            uint32_t time = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
            if (time != 0) {
                CarData *data = CarData_Get();
                if (data) {
                    CarData_Lock(10);
                    if (!data->time_set_manually) {
                        struct timeval tv;
                        tv.tv_sec = time;
                        tv.tv_usec = 0;
                        settimeofday(&tv, NULL);
                        data->systemTime = time;
                        data->time_synced = true;
                        data->time_received_this_boot = true;
                    }
                    CarData_Unlock();
                }
            }
        }
        // Формируем ответ с флагами ожиданий
        uint8_t flags = 0;
        CarData *data = CarData_Get();
        if (data) {
            CarData_Lock(10);
            if (data->trip_stats_pending) flags |= 0x01;
            if (data->day_stats_pending)  flags |= 0x02;
            CarData_Unlock();
        }
        uart_send_to_gateway(MSG_HEARTBEAT_RSP, &flags, 1);
        last_rx_time_us = esp_timer_get_time();
        return;
    }

    // Heartbeat ответ от шлюза
    if (msg_id == MSG_HEARTBEAT_RSP) {
        last_rx_time_us = esp_timer_get_time();
        return;
    }

    // Обнаружение устройств
    if (msg_id == MSG_I_AM_HERE) {
        if (src == ADDR_ESP32_GW && len >= 2 && payload[0] == ADDR_ESP32_GW && payload[1] == DEV_TYPE_GATEWAY) {
            ESP_LOGI(TAG, "Gateway identified via I_AM_HERE");
            CarData *data = CarData_Get();
            if (data) {
                CarData_Lock(10);
                data->uartEsp32Alive = true;
                CarData_Unlock();
            }
        }
        last_rx_time_us = esp_timer_get_time();
        return;
    }

    // Статистика: поездки, сутки, заезды
    if ((msg_id >= 0x35 && msg_id <= 0x3F) || (msg_id >= 0x55 && msg_id <= 0x5A) ||
        (msg_id >= 0x42 && msg_id <= 0x4A) || (msg_id >= 0x5B && msg_id <= 0x64) ||
        msg_id == MSG_DRIVE_CYCLE_DATA || msg_id == MSG_DRIVE_CYCLE_END ||
        (msg_id >= 0x50 && msg_id <= 0x5C && s_in_drive_cycle)) {
        uart_rx_stats_process(msg_id, payload, len);
        last_rx_time_us = esp_timer_get_time();
        return;
    }

    // Данные IMU (акселерометр, гироскоп, наклон, статус калибровки)
    if (msg_id == MSG_REQ_ACCEL || msg_id == MSG_ACCEL_Z || msg_id == MSG_REQ_GYRO ||
        msg_id == MSG_REQ_TILT || msg_id == MSG_REQ_CALIB_STATUS) {
        uart_rx_imu_process(msg_id, payload, len);
        last_rx_time_us = esp_timer_get_time();
        return;
    }

    // Данные от Arduino
    if (src == ADDR_ARDUINO) {
        uart_rx_arduino_process(msg_id, payload, len);
        last_rx_time_us = esp_timer_get_time();
        return;
    }

    // Остальные сообщения (только адресованные нам) – дашборд и служебные
    if (dst != ADDR_ESP32_P4) return;

    if ((msg_id >= 0x20 && msg_id <= 0x32) || msg_id == MSG_TIME || msg_id == MSG_API_RESP || 
        msg_id == MSG_INTERNET_SYNC || msg_id == MSG_LIGHT || msg_id == MSG_LIGHT_SYNTH || 
        msg_id == MSG_LIGHT_RAW) {
        uart_rx_dashboard_process(msg_id, payload, len);
    }
    last_rx_time_us = esp_timer_get_time();
}