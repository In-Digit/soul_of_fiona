#ifndef UART_RX_TASK_H
#define UART_RX_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Задача приёма UART-пакетов (основной цикл)
 */
void uart_rx_task(void *arg);

/**
 * @brief Обработать собранный кадр (диспетчер)
 */
void process_frame(int uart_num, const uint8_t *frame);

#ifdef __cplusplus
}
#endif

#endif // UART_RX_TASK_H