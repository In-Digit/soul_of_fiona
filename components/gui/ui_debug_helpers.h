#ifndef UI_DEBUG_HELPERS_H
#define UI_DEBUG_HELPERS_H

#include "lvgl.h"

/**
 * @brief Эти функции больше не обновляют виджеты – обновление происходит непрерывно в fiona_core.
 * Оставлены для совместимости с вызовами из ui_events.c (обработчики кликов).
 */
void ui_debug_update_arduino_tab(void);
void ui_debug_update_esp32_tab(void);
void ui_debug_update_screen_tab(void);

#endif