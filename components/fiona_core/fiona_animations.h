/**
 * @file fiona_animations.h
 * @brief Публичный API сценариев и визуальных эффектов.
 */
#ifndef FIONA_ANIMATIONS_H
#define FIONA_ANIMATIONS_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Запустить сценарий холодного старта
 */
void fiona_animations_start_cold_boot(void);

/**
 * @brief Применить цветовую тему к фону дашборда
 * @param theme 0=синий, 1=зелёный, 2=красный
 */
void fiona_animations_apply_bg_theme(uint8_t theme);

/**
 * @brief Установить цветовой фильтр для оранжевой иконки (TripImg)
 * @param state 0=исходный оранжевый, 1=зелёный, 2=синий
 */
void fiona_animations_set_tripimg_color(uint8_t state);

/**
 * @brief Установить цветовой фильтр для кольца связи (BlueRing)
 * @param internet_available true = зелёный (связь есть), false = синий (без связи)
 */
void fiona_animations_set_bluering_color(bool internet_available);

#ifdef __cplusplus
}
#endif

#endif // FIONA_ANIMATIONS_H