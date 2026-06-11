/**
 * @file uart_protocol.h
 * @brief Публичный API UART-протокола (асинхронный приём, отправка, обнаружение шлюза).
 */

#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int tx;
    int rx;
} uart_pin_config_t;

/**
 * @brief Инициализировать UART-порты (оба на 921600). После вызова необходимо выполнить
 *        uart_discover_gateway() для определения, на каком порту находится шлюз.
 * @param uart_pins Массив конфигураций пинов (2 элемента).
 * @param count     Количество портов (обычно 2).
 * @return true при успешной инициализации.
 */
bool uart_protocol_init(const uart_pin_config_t *uart_pins, size_t count, int gw_baud, int ard_baud);
/**
 * @brief Деинициализировать UART-порты и освободить ресурсы.
 */
void uart_protocol_deinit(void);

/**
 * @brief Обнаружить шлюз, опрашивая оба порта командой WHO_IS_HERE.
 *        Порт, с которого получен ответ I_AM_HERE, назначается портом шлюза.
 *        Второй порт переводится на скорость 115200 и назначается портом Arduino.
 * @return Номер порта, на котором обнаружен шлюз, или -1 при неудаче.
 */
int uart_discover_gateway(void);

/**
 * @brief Отправить широковещательный пакет (dst = ADDR_BROADCAST).
 */
bool uart_send_broadcast(uint8_t msg_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Отправить пакет данных на шлюз (порт gateway).
 */
bool uart_send_to_gateway(uint8_t msg_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Отправить пакет данных на Arduino (порт arduino).
 */
bool uart_send_to_arduino(uint8_t msg_id, const uint8_t *payload, uint8_t len);

void uart_send_set_fuel_level(float liters);
void uart_send_set_odo(uint32_t odo_km);
void uart_send_refuel_data(float liters_added, float price_per_liter);
void uart_send_full_tank_flag(uint8_t flag);

bool uart_is_gateway_alive(void);
bool uart_is_arduino_alive(void);
uint8_t uart_get_last_rx_msgid(void);

void uart_set_port_baudrate(int uart_num, int baudrate);
void uart_set_arduino_port(int uart_num);
int uart_get_arduino_port(void);
void uart_notify_arduino_rx(void);

#ifdef __cplusplus
}
#endif

#endif // UART_PROTOCOL_H