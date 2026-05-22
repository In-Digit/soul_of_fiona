/**
 * @file uart_protocol.h
 * @brief Публичный API UART-протокола (асинхронный приём, отправка, конфигурация).
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
 * @brief Инициализировать UART-порты (оба на 921600). Позже один из них будет переключён на 115200 для Arduino.
 * @param uart_pins Массив конфигураций пинов (2 элемента).
 * @param count     Количество портов (обычно 2).
 * @return true при успешной инициализации.
 */
bool uart_protocol_init(const uart_pin_config_t *uart_pins, size_t count);

/**
 * @brief Деинициализировать UART-порты и освободить ресурсы.
 */
void uart_protocol_deinit(void);

/**
 * @brief Отправить широковещательный пакет (dst = ADDR_BROADCAST).
 * @param msg_id  Тип сообщения.
 * @param payload Указатель на данные (до 4 байт).
 * @param len     Длина данных (0–4).
 * @return true при успешной отправке.
 */
bool uart_send_broadcast(uint8_t msg_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Отправить пакет данных на шлюз (порт gateway).
 * @param msg_id  Тип сообщения.
 * @param payload Указатель на данные (до 4 байт).
 * @param len     Длина данных (0–4).
 * @return true при успешной отправке.
 */
bool uart_send_to_gateway(uint8_t msg_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Отправить пакет данных на Arduino (порт arduino).
 * @param msg_id  Тип сообщения.
 * @param payload Указатель на данные (до 4 байт).
 * @param len     Длина данных (0–4).
 * @return true при успешной отправке.
 */
bool uart_send_to_arduino(uint8_t msg_id, const uint8_t *payload, uint8_t len);

// ... (остальные объявления из предыдущей версии, плюс новые ниже)

/**
 * @brief Установить остаток топлива (MSG_SET_FUEL_LEVEL).
 */
void uart_send_set_fuel_level(float liters);
void uart_send_set_odo(uint32_t odo_km);
void uart_send_refuel_data(float liters_added, float price_per_liter);
void uart_send_full_tank_flag(uint8_t flag);

/**
 * @brief Проверить, жив ли шлюз (последний успешный приём не старше 6 секунд).
 */
bool uart_is_gateway_alive(void);

/**
 * @brief Проверить, жива ли Arduino (аналогично).
 */
bool uart_is_arduino_alive(void);

uint8_t uart_get_last_rx_msgid(void);

/**
 * @brief Изменить скорость порта (например, для Arduino на 115200).
 */
void uart_set_port_baudrate(int uart_num, int baudrate);

/**
 * @brief Назначить указанный UART портом Arduino и переключить его на 115200.
 */
void uart_set_arduino_port(int uart_num);

/**
 * @brief Получить текущий номер UART-порта, назначенного Arduino.
 * @return Номер порта (UART_NUM_1 или UART_NUM_2) или -1, если ещё не назначен.
 */
int uart_get_arduino_port(void);

/**
 * @brief Обновить временную метку последнего приёма от Arduino.
 */
void uart_notify_arduino_rx(void);

#ifdef __cplusplus
}
#endif

#endif // UART_PROTOCOL_H