/**
 * @file uart_rx_arduino.c
 * @brief Обработчик данных от Arduino (заглушка для будущей реализации)
 */

#include "uart_rx_arduino.h"
#include "esp_log.h"

static const char *TAG = "UART_ARDUINO";

void uart_rx_arduino_process(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    // Пока Arduino не подключен, ничего не делаем
    ESP_LOGD(TAG, "Arduino packet received: msg_id=0x%02X, len=%d (ignored)", msg_id, len);
}