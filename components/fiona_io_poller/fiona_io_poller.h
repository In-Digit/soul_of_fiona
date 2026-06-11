/**
 * @file fiona_io_poller.h
 * @brief Фоновая задача последовательного опроса шлюза и Arduino.
 *
 * Не зависит от GUI. Запускается один раз после инициализации UART.
 * Периодически запрашивает все необходимые параметры, поддерживая
 * CarData в актуальном состоянии.
 */

#ifndef FIONA_IO_POLLER_H
#define FIONA_IO_POLLER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Создать и запустить задачу опроса.
 *        Должна вызываться после uart_protocol_init().
 */
void fiona_io_poller_start(void);

#ifdef __cplusplus
}
#endif

#endif // FIONA_IO_POLLER_H