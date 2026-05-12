#ifndef UART_RX_DASHBOARD_H
#define UART_RX_DASHBOARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Обработать пакет с данными для дашборда (скорость, обороты, температура и т.д.)
 * @param msg_id  Идентификатор сообщения
 * @param payload Указатель на данные (0-4 байта)
 * @param len     Длина данных
 */
void uart_rx_dashboard_process(uint8_t msg_id, const uint8_t *payload, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif // UART_RX_DASHBOARD_H