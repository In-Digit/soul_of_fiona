#ifndef UART_RX_STATS_H
#define UART_RX_STATS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Флаг для различения контекста в process_frame
extern bool s_in_drive_cycle;

/**
 * @brief Обработать пакет, относящийся к статистике (рейсы, сутки, заезды)
 * @param msg_id  Идентификатор сообщения
 * @param payload Указатель на данные (0-4 байта)
 * @param len     Длина данных
 */
void uart_rx_stats_process(uint8_t msg_id, const uint8_t *payload, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif // UART_RX_STATS_H