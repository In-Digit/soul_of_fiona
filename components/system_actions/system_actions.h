#ifndef SYSTEM_ACTIONS_H
#define SYSTEM_ACTIONS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Установка времени из текстового поля (обработчик SetTimeClicked)
void system_actions_set_time_from_textarea(void);

// Запрос времени у шлюза (обработчик SetTimeRClicked)
void system_actions_request_time(void);

#ifdef __cplusplus
}
#endif

#endif