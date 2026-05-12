#include "system_actions.h"
#include "CarData.h"
#include "uart_protocol.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// Внешний объект UI
extern lv_obj_t * ui_System_Textarea_TimeTextArea;
// Таймер часов (объявлен в fiona_core)
extern lv_timer_t * clock_timer;

void system_actions_set_time_from_textarea(void) {
    const char *text = lv_textarea_get_text(ui_System_Textarea_TimeTextArea);
    if (!text || strlen(text) < 3) return;

    int values[3] = {0}, count = 0, num_pos = 0;
    char num_buf[8];
    for (int i = 0; text[i] != '\0' && count < 3; i++) {
        if (text[i] >= '0' && text[i] <= '9') {
            if (num_pos < (int)sizeof(num_buf)-1) num_buf[num_pos++] = text[i];
        } else {
            if (num_pos > 0) { num_buf[num_pos] = '\0'; values[count++] = atoi(num_buf); num_pos = 0; }
        }
    }
    if (num_pos > 0 && count < 3) { num_buf[num_pos] = '\0'; values[count++] = atoi(num_buf); }

    if (count >= 3) {
        int hour = values[0], min = values[1], sec = values[2];
        if (hour >= 0 && hour <= 23 && min >= 0 && min <= 59 && sec >= 0 && sec <= 59) {
            struct tm tm;
            time_t now = time(NULL);
            localtime_r(&now, &tm);
            tm.tm_hour = hour; tm.tm_min = min; tm.tm_sec = sec;
            struct timeval tv = { .tv_sec = mktime(&tm), .tv_usec = 0 };
            settimeofday(&tv, NULL);
            if (clock_timer) lv_timer_reset(clock_timer);

            // Взвести флаг ручной установки времени
            CarData *data = CarData_Get();
            if (data) {
                CarData_Lock(10);
                data->time_set_manually = true;
                CarData_Unlock();
            }
        }
    }
}

void system_actions_request_time(void) {
    uart_send_to_gateway(MSG_REQ_TIME, NULL, 0);
}