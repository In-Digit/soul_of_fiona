/**
 * @file UARTBinary.h
 * @brief Функции для работы с бинарным протоколом по UART2.
 */

#ifndef UARTBINARY_H
#define UARTBINARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализирует UART2 для связи с Экран.
 * @param baud_rate Скорость (921600).
 * @param rx_pin Номер пина RX (по умолчанию 16).
 * @param tx_pin Номер пина TX (по умолчанию 17).
 */
void uart_binary_init(unsigned long baud_rate, int rx_pin, int tx_pin);

/**
 * @brief Отправляет бинарный кадр.
 * @param msg_id Идентификатор сообщения.
 * @param src Адрес отправителя (обычно ADDR_ESP32_GW).
 * @param dst Адрес получателя (ADDR_ESP32_P4).
 * @param payload Указатель на данные.
 * @param len Длина значащих данных (0-4).
 */
void uart_send_packet(uint8_t msg_id, uint8_t src, uint8_t dst, const uint8_t* payload, uint8_t len);

/**
 * @brief Вызывается в цикле uartTask. Обрабатывает входящие данные и отправляет периодические пакеты.
 */
void uart_task_process(void);

/**
 * @brief Устанавливает callback для обработки принятых команд.
 * @param callback Функция, принимающая MsgID и payload.
 */
typedef void (*uart_command_callback_t)(uint8_t msg_id, const uint8_t* payload, uint8_t len);
void uart_set_command_callback(uart_command_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // UARTBINARY_H