/**
 * @file uart_protocol.h
 * @brief Публичный API UART-протокола (асинхронный приём, отправка).
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
 * @brief Инициализировать UART-порты с фиксированными пинами и скоростью 115200.
 * @param uart_pins Массив конфигураций пинов (2 элемента: UART1 для шлюза, UART2 для Arduino).
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
 * @brief Отправить пакет данных на шлюз.
 * @param msg_id  Тип сообщения.
 * @param payload Указатель на данные (до 4 байт).
 * @param len     Длина данных (0–4).
 * @return true при успешной отправке.
 */
bool uart_send_to_gateway(uint8_t msg_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Установить остаток топлива (MSG_SET_FUEL_LEVEL, 0x53).
 * @param liters Текущий остаток топлива в литрах (будет умножен на 100 и отправлен как uint16 в little-endian).
 */
void uart_send_set_fuel_level(float liters);

/**
 * @brief Установить одометр (MSG_SET_ODO, 0x54).
 * @param odo_km Пробег в километрах (отправляется как uint32 в little-endian).
 */
void uart_send_set_odo(uint32_t odo_km);

/**
 * @brief Отправить данные заправки (MSG_REFUEL_DATA, 0x50).
 * @param liters_added Количество добавленного топлива в литрах (умножается на 10).
 * @param price_per_liter Цена за литр в рублях (умножается на 100).
 */
void uart_send_refuel_data(float liters_added, float price_per_liter);

/**
 * @brief Отправить флаг полного бака (MSG_FULL_TANK_FLAG, 0x52).
 * @param flag 0 или 1.
 */
void uart_send_full_tank_flag(uint8_t flag);

/**
 * @brief Проверить, была ли активность на линии (получен любой пакет) за последние 6 секунд.
 * @return true, если связь есть.
 */
bool uart_is_gateway_alive(void);

/**
 * @brief Получить MsgID последнего успешно принятого кадра.
 * @return MsgID последнего принятого кадра (0, если не было).
 */
uint8_t uart_get_last_rx_msgid(void);

#ifdef __cplusplus
}
#endif

#endif // UART_PROTOCOL_H