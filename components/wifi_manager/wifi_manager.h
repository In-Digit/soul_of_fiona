/**
 * @file wifi_manager.h
 * @brief Минимальный компонент для подключения к Wi‑Fi через ESP‑Hosted (C6).
 *
 * Предоставляет только автоматическое подключение с фолбэком и получение IP.
 * Все операции потокобезопасны, не используют фоновых задач.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Подключиться к Wi‑Fi по лучшему сценарию (сохранённая сеть, Primary, Secondary).
 *        Вызывается один раз до старта LVGL.
 * @return ESP_OK при успехе.
 */
esp_err_t wifi_manager_connect_best(void);

/**
 * @brief Получить текущий IP-адрес (только для чтения).
 * @return Строка вида "192.168.1.100" или "0.0.0.0".
 */
const char* wifi_manager_get_ip(void);

#ifdef __cplusplus
}
#endif

#endif