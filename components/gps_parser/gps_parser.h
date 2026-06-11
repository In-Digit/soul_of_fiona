/**
 * @file gps_parser.h
 * @brief Лёгкий парсер NMEA-строк для GY-GPS6MV2 (u-blox NEO-6M).
 *
 * Предоставляет инициализацию UART, фоновую задачу для приёма данных
 * и обновление координат/скорости/времени в CarData.
 */

#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализировать GPS-модуль на заданных пинах.
 * @return ESP_OK при успешной инициализации.
 */
esp_err_t gps_init(int tx_pin, int rx_pin, int baud_rate);
/**
 * @brief Проверить, был ли получен хотя бы один валидный пакет.
 */
bool gps_is_alive(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_PARSER_H