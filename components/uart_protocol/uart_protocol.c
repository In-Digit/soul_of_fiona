/**
 * @file uart_protocol.c
 * @brief Реализация публичного API UART-протокола (инициализация, отправка, статус).
 *
 * Поддерживает два порта: gateway (шлюз) и arduino (вентиляторы радиатора).
 * После обнаружения шлюза порт Arduino переводится на 115200 бод.
 */

#include "uart_protocol.h"
#include "uart_rx_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "UART_PROTO";

#define MAX_UART_PORTS 2
#define UART_BAUD_RATE 921600
#define UART_BAUD_ARDUINO 115200
#define UART_RX_BUFFER_SIZE 1024

static int gateway_uart_num = -1;
static int arduino_uart_num = -1;
volatile int64_t last_rx_time_us = 0;
volatile int64_t last_rx_arduino_time_us = 0;
volatile uint8_t last_rx_msg_id = 0;

// Предварительное объявление внутренней функции отправки кадра
static bool uart_send_frame(int uart_num, uint8_t msg_id, uint8_t dst, const uint8_t *payload, uint8_t len);

// Внешние функции из uart_rx_task.c
extern void uart_rx_task(void *arg);

uint8_t crc8_calculate(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool uart_protocol_init(const uart_pin_config_t *uart_pins, size_t count) {
    if (count > MAX_UART_PORTS) count = MAX_UART_PORTS;

    for (size_t i = 0; i < count; i++) {
        uart_config_t uart_config = {
            .baud_rate = UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        int uart_num = (i == 0) ? UART_NUM_1 : UART_NUM_2;
        ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_RX_BUFFER_SIZE, 0, 0, NULL, 0));
        ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(uart_num, uart_pins[i].tx, uart_pins[i].rx,
                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

        if (i == 0) {
            gateway_uart_num = uart_num;
        } else {
            arduino_uart_num = uart_num;  // временно, до обнаружения шлюза
        }
        ESP_LOGI(TAG, "UART%d initialized (TX:%d, RX:%d) at %d baud",
                 uart_num, uart_pins[i].tx, uart_pins[i].rx, UART_BAUD_RATE);
    }

    xTaskCreate(uart_rx_task, "uart_rx", 4096, (void*)(uintptr_t)count, 1, NULL);

    last_rx_time_us = esp_timer_get_time();
    last_rx_arduino_time_us = esp_timer_get_time();
    return true;
}

void uart_protocol_deinit(void) {
    if (gateway_uart_num >= 0) {
        uart_driver_delete(gateway_uart_num);
        gateway_uart_num = -1;
    }
    if (arduino_uart_num >= 0) {
        uart_driver_delete(arduino_uart_num);
        arduino_uart_num = -1;
    }
}

bool uart_send_broadcast(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    if (gateway_uart_num < 0) return false;
    return uart_send_frame(gateway_uart_num, msg_id, ADDR_BROADCAST, payload, len);
}

bool uart_send_to_gateway(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    if (gateway_uart_num < 0) return false;
    return uart_send_frame(gateway_uart_num, msg_id, ADDR_ESP32_GW, payload, len);
}

bool uart_send_to_arduino(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    if (arduino_uart_num < 0) return false;
    return uart_send_frame(arduino_uart_num, msg_id, ADDR_ARDUINO, payload, len);
}

static bool uart_send_frame(int uart_num, uint8_t msg_id, uint8_t dst, const uint8_t *payload, uint8_t len) {
    if (len > FRAME_PAYLOAD_SIZE) return false;
    uint8_t frame[FRAME_TOTAL_SIZE];
    frame[0] = FRAME_MAGIC;
    frame[1] = msg_id;
    frame[2] = ADDR_ESP32_P4;
    frame[3] = dst;
    frame[4] = len;
    memset(&frame[5], 0, FRAME_PAYLOAD_SIZE);
    if (payload && len > 0) {
        memcpy(&frame[5], payload, len);
    }
    frame[FRAME_TOTAL_SIZE - 1] = crc8_calculate(frame, FRAME_TOTAL_SIZE - 1);
    int written = uart_write_bytes(uart_num, (const char*)frame, FRAME_TOTAL_SIZE);
    return (written == FRAME_TOTAL_SIZE);
}

void uart_send_set_fuel_level(float liters) {
    uint16_t val = (uint16_t)(liters * 100.0f);
    uint8_t payload[2];
    payload[0] = val & 0xFF;
    payload[1] = (val >> 8) & 0xFF;
    uart_send_to_gateway(MSG_SET_FUEL_LEVEL, payload, 2);
}

void uart_send_set_odo(uint32_t odo_km) {
    uint8_t payload[4];
    payload[0] = odo_km & 0xFF;
    payload[1] = (odo_km >> 8) & 0xFF;
    payload[2] = (odo_km >> 16) & 0xFF;
    payload[3] = (odo_km >> 24) & 0xFF;
    uart_send_to_gateway(MSG_SET_ODO, payload, 4);
}

void uart_send_refuel_data(float liters_added, float price_per_liter) {
    uint16_t lit = (uint16_t)(liters_added * 10.0f);
    uint16_t price = (uint16_t)(price_per_liter * 100.0f);
    uint8_t payload[4];
    payload[0] = lit & 0xFF;
    payload[1] = (lit >> 8) & 0xFF;
    payload[2] = price & 0xFF;
    payload[3] = (price >> 8) & 0xFF;
    uart_send_to_gateway(MSG_REFUEL_DATA, payload, 4);
}

void uart_send_full_tank_flag(uint8_t flag) {
    uart_send_to_gateway(MSG_FULL_TANK_FLAG, &flag, 1);
}

bool uart_is_gateway_alive(void) {
    int64_t now = esp_timer_get_time();
    return (now - last_rx_time_us) < 6000000;
}

bool uart_is_arduino_alive(void) {
    int64_t now = esp_timer_get_time();
    return (now - last_rx_arduino_time_us) < 6000000;
}

uint8_t uart_get_last_rx_msgid(void) {
    return last_rx_msg_id;
}

/**
 * @brief Переключить скорость UART-порта.
 * @param uart_num Номер порта (UART_NUM_1 или UART_NUM_2).
 * @param baudrate Желаемая скорость.
 */
void uart_set_port_baudrate(int uart_num, int baudrate) {
    uart_set_baudrate(uart_num, baudrate);
    ESP_LOGI(TAG, "UART%d baudrate changed to %d", uart_num, baudrate);
}

/**
 * @brief Зафиксировать порт Arduino и переключить его на 115200.
 * @param uart_num Номер порта, на котором обнаружена Arduino.
 */
void uart_set_arduino_port(int uart_num) {
    arduino_uart_num = uart_num;
    uart_set_port_baudrate(arduino_uart_num, UART_BAUD_ARDUINO);
    ESP_LOGI(TAG, "Arduino assigned to UART%d", arduino_uart_num);
}

/**
 * @brief Получить текущий номер UART-порта, назначенного Arduino.
 * @return Номер порта или -1, если ещё не назначен.
 */
int uart_get_arduino_port(void) {
    return arduino_uart_num;
}

/**
 * @brief Обновить временную метку последнего приёма от Arduino.
 * Вызывается из uart_rx_arduino при успешном приёме пакета.
 */
void uart_notify_arduino_rx(void) {
    last_rx_arduino_time_us = esp_timer_get_time();
}