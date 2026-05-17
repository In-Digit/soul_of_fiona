#ifndef UART_RX_IMU_H
#define UART_RX_IMU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_rx_imu_process(uint8_t msg_id, const uint8_t *payload, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif