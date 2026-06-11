/**
 * @file rtc.h
 * @brief Драйвер внешнего RTC RX8025 (I2C).
 *
 * Предоставляет функции для инициализации, чтения и записи времени
 * в независимый чип реального времени, подключённый по шине I2C.
 */

#ifndef RTC_H
#define RTC_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Структура для хранения времени, считанного с RTC
 */
typedef struct {
    int sec;   // Секунды (0-59)
    int min;   // Минуты (0-59)
    int hour;  // Часы (0-23)
    int day;   // День (1-31)
    int mon;   // Месяц (1-12)
    int year;  // Год (от 2000)
} rtc_time_t;

/**
 * @brief Инициализирует RTC RX8025.
 * @return ESP_OK в случае успеха, иначе код ошибки.
 */
esp_err_t rtc_init(void);

/**
 * @brief Читает текущее время с RTC.
 * @param[out] time Указатель на структуру, куда будет записано время.
 * @return ESP_OK в случае успеха, иначе код ошибки.
 */
esp_err_t rtc_get_time(rtc_time_t *time);

/**
 * @brief Устанавливает время на RTC.
 * @param[in] time Указатель на структуру с новым временем.
 * @return ESP_OK в случае успеха, иначе код ошибки.
 */
esp_err_t rtc_set_time(const rtc_time_t *time);

#ifdef __cplusplus
}
#endif

#endif // RTC_H