/**
 * @file fiona_core.h
 * @brief Публичный API ядра дашборда Фионы.
 */

#ifndef FIONA_CORE_H
#define FIONA_CORE_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Субъекты LVGL (глобально доступны)
extern lv_subject_t subject_speed;
extern lv_subject_t subject_rpm;
extern lv_subject_t subject_bat;
extern lv_subject_t subject_fuel;
extern lv_subject_t subject_temp;
extern lv_subject_t subject_odo;
extern lv_subject_t subject_range;
extern lv_subject_t subject_lph;
extern lv_subject_t subject_trip_time;
extern lv_subject_t subject_trip_pause;
extern lv_subject_t subject_trip_fuel;
extern lv_subject_t subject_trip_dist;
extern lv_subject_t subject_clock_color;
extern lv_subject_t subject_dash_message;
extern lv_subject_t subject_adc_light;
extern lv_subject_t subject_throttle;
extern lv_subject_t subject_accel;

// Таймеры
extern lv_timer_t * poll_timer;
extern lv_timer_t * clock_timer;
extern bool screensaver_active;

// Инициализация ядра (вызывается из main)
void fiona_core_init(void);

// Сценарии (вызываются из ui_events)
void fiona_core_dashboard_on_load(void);
void fiona_core_request_internet_time(void);
void fiona_core_activate_screensaver(void);
void fiona_core_deactivate_screensaver(void);

// Обновление дашборда
void fiona_core_refresh_dashboard(void);

// Сохранение / загрузка CarData в NVS (используется и вне ядра)
void fiona_core_save_car_data_to_nvs(void);
void fiona_core_load_car_data_from_nvs(void);

// Установка цвета часов
void fiona_core_set_clock_color(int color);

#ifdef __cplusplus
}
#endif

#endif // FIONA_CORE_H