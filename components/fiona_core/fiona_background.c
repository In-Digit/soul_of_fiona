/**
 * @file fiona_background.c
 * @brief Фоновые таймеры и периодическая логика ядра.
 */

#include "fiona_core.h"
#include "fiona_animations.h"
#include "CarData.h"
#include "fiona_brain.h"
#include "fiona_soul.h"
#include "fiona_phrase_loader.h"
#include "uart_protocol.h"
#include "sd_utils.h"
#include "esp_log.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "bsp/esp-bsp.h"

/* ========== Внешние UI-объекты ========== */
extern lv_obj_t * ui_DashBoard_Label_SpeedDigit;
extern lv_obj_t * ui_DashBoard_Arc_Taho;
extern lv_obj_t * ui_DashBoard_Bar_BatBar;
extern lv_obj_t * ui_DashBoard_Label_AKB;
extern lv_obj_t * ui_DashBoard_Bar_FuelBar;
extern lv_obj_t * ui_DashBoard_Label_FUEL;
extern lv_obj_t * ui_DashBoard_Bar_TempBar;
extern lv_obj_t * ui_DashBoard_Label_TempNum;
extern lv_obj_t * ui_DashBoard_Label_ODO;
extern lv_obj_t * ui_DashBoard_Label_ODOonFuel;
extern lv_obj_t * ui_DashBoard_Bar_lph;
extern lv_obj_t * ui_DashBoard_Label_LPHnum;
extern lv_obj_t * ui_DashBoard_Label_TripTime;
extern lv_obj_t * ui_DashBoard_Label_TripTimePause;
extern lv_obj_t * ui_DashBoard_Label_TripNum;
extern lv_obj_t * ui_DashBoard_Label_TripLitr;
extern lv_obj_t * ui_DashBoard_Label_Time;
extern lv_obj_t * ui_DashBoard_Label_TahoLBL;
extern lv_obj_t * ui_DashBoard_Arc_Drossel;
extern lv_obj_t * ui_DashBoard_Arc_Uskorenie;
extern lv_obj_t * ui_DashBoard_Label_GazBL;
extern lv_obj_t * ui_DashBoard_Label_AxelBL;
extern lv_obj_t * ui_DashBoard_Image_BlueRing;
extern lv_obj_t * ui_DashBoard_Image_Batallert;
extern lv_obj_t * ui_DashBoard_Image_FuelAllert;
extern lv_obj_t * ui_DashBoard_Image_CoolNorm;
extern lv_obj_t * ui_DashBoard_Image_CoolHigh;
extern lv_obj_t * ui_DashBoard_Image_TripImg;
extern lv_obj_t * ui_DashBoard_Image_FreeWind;
extern lv_obj_t * ui_DashBoard_Image_ConditionHot;
extern lv_obj_t * ui_DashBoard_Image_ConditionCold;
extern lv_obj_t * ui_DashBoard_Image_RFirstVent;
extern lv_obj_t * ui_DashBoard_Image_RFirstVentAlert;
extern lv_obj_t * ui_DashBoard_Image_RSecondVent;
extern lv_obj_t * ui_DashBoard_Image_RSecondVentAlert;
extern lv_obj_t * ui_DashBoard_Image_RSecondVentAlert1;
extern lv_obj_t * ui_DashBoard_Label_FionaSpeachLabelDash;
extern lv_obj_t * ui_DashBoard_Label_EditionString;
extern lv_obj_t * ui_Screen_DashBoard;
extern lv_obj_t * ui_Screen_SplashScreen;
extern lv_obj_t * ui_SplashScreen_Label_Clock;
extern lv_obj_t * ui_SplashScreen_Label_FionaSpeachLabel;
extern lv_obj_t * ui_System_Textarea_TextAreaArduino;
extern lv_obj_t * ui_System_Textarea_TextAreaESP;
extern lv_obj_t * ui_System_Textarea_TextAreaScreen;
extern lv_obj_t * ui_System_Label_CurrentLight;
extern lv_obj_t * ui_System_Image_SDBack;
extern lv_obj_t * ui_System_Image_SDGrey;
extern lv_obj_t * ui_System_Image_SDRed;
extern lv_obj_t * ui_System_Image_ArduinoBack;
extern lv_obj_t * ui_System_Image_ArduinoGrey;
extern lv_obj_t * ui_System_Image_ArduinoRed;
extern lv_obj_t * ui_System_Image_ESP32Back;
extern lv_obj_t * ui_System_Image_ESP32Grey;
extern lv_obj_t * ui_System_Image_ESP32Red;
extern lv_obj_t * ui_System_Image_SpeachBack;
extern lv_obj_t * ui_System_Image_SpeachGrey;
extern lv_obj_t * ui_System_Image_SpeachRed;
extern lv_obj_t * ui_System_Image_StatBack;
extern lv_obj_t * ui_System_Image_StatGrey;
extern lv_obj_t * ui_System_Image_StatRed;
extern lv_obj_t * ui_System_Image_PresetBack;
extern lv_obj_t * ui_System_Image_PresetGrey;
extern lv_obj_t * ui_System_Image_PresetRed;

extern lv_timer_t *poll_timer;
extern lv_timer_t *clock_timer;
extern bool screensaver_active;

// -------------------- Локальные переменные --------------------
static uint32_t engine_off_seconds = 0;
static bool show_trip_cost = false;

static char esp32_status_text[512] = {0};
static char arduino_status_text[512] = {0};
static char screen_status_text[512] = {0};
static bool indicators_ok[6] = {false};

uint8_t g_current_tone = 0;

// Для подсчёта остановок: кольцевой буфер моментов остановок (Unix time)
#define STOP_HISTORY_SIZE 8
static uint32_t stop_history[STOP_HISTORY_SIZE] = {0};
static int stop_history_idx = 0;

// Для ограничения частоты вывода фраз
static uint32_t last_important_phrase_time = 0;
static uint32_t last_casual_phrase_time = 0;
static uint32_t phrase_display_start_time = 0;
static char last_displayed_text[256] = {0};

// Прототипы
static void fast_timer_cb(lv_timer_t *timer);
static void clock_timer_cb(lv_timer_t *timer);
static void update_stop_history(uint32_t now);
static int count_stops_last_5min(uint32_t now);

void fiona_background_init_timers(void) {
    if (clock_timer == NULL) clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    if (poll_timer == NULL)   poll_timer   = lv_timer_create(fast_timer_cb, 100, NULL);
}

/* ---------- Подсчёт остановок ---------- */
static void update_stop_history(uint32_t now) {
    stop_history[stop_history_idx] = now;
    stop_history_idx = (stop_history_idx + 1) % STOP_HISTORY_SIZE;
}

static int count_stops_last_5min(uint32_t now) {
    int count = 0;
    for (int i = 0; i < STOP_HISTORY_SIZE; i++) {
        if (stop_history[i] != 0 && (now - stop_history[i]) <= 300) {
            count++;
        }
    }
    return count;
}

/* ---------- Быстрый таймер (100 мс) ---------- */
static void fast_timer_cb(lv_timer_t *timer) {
    CarData *data = CarData_Get();
    if (!data || !uart_is_gateway_alive()) {
        return;
    }

    static uint8_t fast_counter = 0;
    fast_counter++;

    // RPM запрашиваем каждые 500 мс (каждый 5-й тик)
    if (fast_counter % 5 == 0) {
        uart_send_to_gateway(MSG_RPM, NULL, 0);
    }

    // Speed и Instant Fuel – каждые 200 мс (каждый 2-й тик)
    if (fast_counter % 2 == 0) {
        uart_send_to_gateway(MSG_SPEED, NULL, 0);
        uart_send_to_gateway(MSG_INST_FUEL, NULL, 0);
    }

    // Абсолютное положение дросселя – каждые 500 мс
    if (fast_counter % 5 == 0) {
        uart_send_to_gateway(MSG_THROTTLE, NULL, 0);
    }

    if (fast_counter >= 10) fast_counter = 0;

    int speed_val, rpm_val, lph_val, throttle_val, accel_val;
    CarData_Lock(10);
    speed_val = data->speedValue;
    rpm_val = data->rpmValue;
    lph_val = (int)(data->lphValue * 10);
    throttle_val = (int)data->throttlePos;
    accel_val = 0;   // заглушка
    CarData_Unlock();

    lv_subject_set_int(&subject_speed, speed_val);
    lv_subject_set_int(&subject_rpm, rpm_val);
    lv_subject_set_int(&subject_lph, lph_val);
    lv_subject_set_int(&subject_throttle, throttle_val);
    lv_subject_set_int(&subject_accel, accel_val);
}

/* ---------- Медленный таймер (1 сек) ---------- */
static void clock_timer_cb(lv_timer_t *timer) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *timeinfo = localtime(&tv.tv_sec);
    char buf[64];
    uint32_t now = (uint32_t)tv.tv_sec;

    CarData *data = CarData_Get();
    if (data) {
        CarData_Lock(10);
        data->systemTime = now;
        if (data->rpmValue > 400) {
            data->last_valid_coolant_temp = data->tempValue;
        }
        // Управление длительностью заезда и остановками
        FionaState *state = fiona_brain_get_state();
        if (state) {
            if (data->rpmValue > 400) {
                if (state->engine_start_time == 0) {
                    state->engine_start_time = now;
                }
                state->trip_duration_sec = now - state->engine_start_time;
            } else {
                state->engine_start_time = 0;
                state->trip_duration_sec = 0;
                state->long_trip = false;
                state->very_long_trip = false;
            }
            if (data->speedValue == 0 && data->rpmValue == 0) {
                update_stop_history(now);
            }
        }
        CarData_Unlock();
    }

    // Часы
    strftime(buf, sizeof(buf), "%H:%M:%S", timeinfo);
    lv_label_set_text(ui_DashBoard_Label_Time, buf);
    if (screensaver_active || lv_scr_act() == ui_Screen_SplashScreen) {
        const char *days[] = {"Воскресенье","Понедельник","Вторник","Среда","Четверг","Пятница","Суббота"};
        strftime(buf, sizeof(buf), "%d.%m.%Y года\n%H:%M:%S\n", timeinfo);
        strcat(buf, days[timeinfo->tm_wday]);
        lv_label_set_text(ui_SplashScreen_Label_Clock, buf);
    }

    if (!data) return;

    uart_send_to_gateway(MSG_LIGHT_RAW, NULL, 0);
    if (timeinfo->tm_hour == 0 && timeinfo->tm_min == 1 && timeinfo->tm_sec == 0) {
        if (!data->day_requested_today) {
            uart_send_to_gateway(MSG_REQ_DAY_STATS, NULL, 0);
            CarData_Lock(10);
            data->day_requested_today = true;
            CarData_Unlock();
        }
    }
    if (timeinfo->tm_hour == 0 && timeinfo->tm_min == 0 && timeinfo->tm_sec == 1) {
        CarData_Lock(10);
        data->day_requested_today = false;
        CarData_Unlock();
    }

    // Обновляем stop_count_5min в мозгу
    FionaState *state = fiona_brain_get_state();
    if (state) {
        state->stop_count_5min = count_stops_last_5min(now);
    }

    fiona_brain_update();

    // Получаем фразу и выводим с ограничением по частоте
    state = fiona_brain_get_state();
    if (state) {
        FionaPhrase phrase = fiona_soul_get_phrase(state);
        bool show = false;

        if (phrase.text && phrase.text[0] != '\0') {
            if (phrase.tone == FIONA_TONE_SERIOUS) {
                if (now - last_important_phrase_time >= 10) {
                    show = true;
                    last_important_phrase_time = now;
                }
            } else {
                uint8_t mode = state->driving_mode;
                int min_interval = 10;
                if (mode == 2) min_interval = 120;
                else if (mode == 1) min_interval = 30;
                else if (mode == 3) min_interval = 15;

                if (now - last_casual_phrase_time >= min_interval) {
                    show = true;
                    last_casual_phrase_time = now;
                }
            }
        }

        if (show) {
            lv_subject_set_pointer(&subject_dash_message, (void*)phrase.text);
            g_current_tone = phrase.tone;
            lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "");
            phrase_display_start_time = now;
            strncpy(last_displayed_text, phrase.text, sizeof(last_displayed_text)-1);
        } else {
            if (phrase_display_start_time > 0 && (now - phrase_display_start_time) > 10) {
                lv_subject_set_pointer(&subject_dash_message, (void*)"");
                lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "");
                phrase_display_start_time = 0;
                last_displayed_text[0] = '\0';
            }
        }

                // ---------- ЦВЕТОВЫЕ МАСКИ (отключены, ждут реализации) ----------
        // uint8_t bg_theme = 0; // синий по умолчанию
        // if (!state->is_alone || state->trip_duration_sec > 10800) {
        //     bg_theme = 1; // зелёный
        // } else if (state->is_alone && state->driving_mode != 2) {
        //     bg_theme = 2; // красный
        // }
        // fiona_animations_apply_bg_theme(bg_theme);
        // fiona_animations_set_tripimg_color(1); // TODO: различать ручной/авто
        // fiona_animations_set_bluering_color(data->internetAvailable);

        // BlueRing: интернет
        //fiona_animations_set_bluering_color(data->internetAvailable);
    }

    // Автояркость (полный код)
    CarData_Lock(10);
    uint16_t ambient_raw = data->ambient_light_raw;
    uint8_t max_brightness = data->backlight_brightness;
    uint8_t min_br = data->backlight_min_brightness;
    uint8_t dark_thr = data->light_threshold_dark;
    uint8_t bright_thr = data->light_threshold_bright;
    CarData_Unlock();

    static uint8_t last_brightness = 255;
    uint8_t new_duty;
    if (dark_thr < bright_thr) {
        if (ambient_raw <= dark_thr) {
            new_duty = min_br;
        } else if (ambient_raw >= bright_thr) {
            new_duty = max_brightness;
        } else {
            float norm = (float)(ambient_raw - dark_thr) / (float)(bright_thr - dark_thr);
            float curved = powf(norm, 0.5f);
            new_duty = min_br + (uint8_t)((max_brightness - min_br) * curved);
        }
    } else {
        float norm = (float)ambient_raw / 4095.0f;
        float curved = powf(norm, 0.5f);
        new_duty = min_br + (uint8_t)((max_brightness - min_br) * curved);
    }
    if (new_duty > 100) new_duty = 100;
    if (new_duty < min_br) new_duty = min_br;
    if (new_duty != last_brightness) {
        bsp_display_brightness_set(new_duty);
        last_brightness = new_duty;
    }
    lv_subject_set_int(&subject_adc_light, ambient_raw);

    // Отладочные индикаторы и текстовые поля
    CarData_Lock(10);
    float bat_voltage = data->batValue;
    float fuel_level = data->fuelValue;
    int temp_coolant = data->tempValue;
    int speed_kmh = data->speedValue;
    int current_rpm = data->rpmValue;
    uint32_t odo_val = data->odoKm;
    bool arduino_alive = data->uartArduinoAlive;
    bool trip_state = data->tripState;
    CarData_Unlock();

    snprintf(esp32_status_text, sizeof(esp32_status_text),
             "Время: %02d:%02d:%02d\n"
             "Скорость: %d км/ч\n"
             "Тахометр: %d об/мин\n"
             "АКБ: %.1f В\n"
             "Топливо: %.1f л\n"
             "ОДО: %u км\n",
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
             speed_kmh, current_rpm,
             bat_voltage, fuel_level, odo_val);
    snprintf(arduino_status_text, sizeof(arduino_status_text),
             "Статус: %s", arduino_alive ? "На связи" : "Нет связи");

    static uint32_t uptime_seconds = 0;
    uptime_seconds++;
    snprintf(screen_status_text, sizeof(screen_status_text),
             "Время работы: %lu сек\n"
             "SD: %s\n"
             "Arduino: %s\n"
             "ESP32: %s\n",
             uptime_seconds,
             indicators_ok[0] ? "OK" : "Ошибка",
             indicators_ok[1] ? "OK" : "Нет связи",
             indicators_ok[2] ? "OK" : "Нет связи");

    lv_textarea_set_text(ui_System_Textarea_TextAreaArduino, arduino_status_text);
    lv_textarea_set_text(ui_System_Textarea_TextAreaESP, esp32_status_text);
    lv_textarea_set_text(ui_System_Textarea_TextAreaScreen, screen_status_text);

    // Индикаторы (мигание)
    static bool blink_toggle = false;
    blink_toggle = !blink_toggle;
    struct {
        lv_obj_t *back;
        lv_obj_t *grey;
        lv_obj_t *red;
    } indicator_widgets[6] = {
        { ui_System_Image_SDBack, ui_System_Image_SDGrey, ui_System_Image_SDRed },
        { ui_System_Image_ArduinoBack, ui_System_Image_ArduinoGrey, ui_System_Image_ArduinoRed },
        { ui_System_Image_ESP32Back, ui_System_Image_ESP32Grey, ui_System_Image_ESP32Red },
        { ui_System_Image_SpeachBack, ui_System_Image_SpeachGrey, ui_System_Image_SpeachRed },
        { ui_System_Image_StatBack, ui_System_Image_StatGrey, ui_System_Image_StatRed },
        { ui_System_Image_PresetBack, ui_System_Image_PresetGrey, ui_System_Image_PresetRed }
    };
    for (int i = 0; i < 6; i++) {
        bool ok = indicators_ok[i];
        lv_obj_add_flag(indicator_widgets[i].back, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(indicator_widgets[i].grey, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(indicator_widgets[i].red, LV_OBJ_FLAG_HIDDEN);
        if (ok) {
            lv_obj_remove_flag(indicator_widgets[i].back, LV_OBJ_FLAG_HIDDEN);
        } else {
            if (blink_toggle) {
                lv_obj_remove_flag(indicator_widgets[i].red, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(indicator_widgets[i].grey, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    // Запросы к шлюзу (раз в секунду)
    if (!screensaver_active && lv_scr_act() == ui_Screen_DashBoard && uart_is_gateway_alive()) {
        uart_send_to_gateway(MSG_COOLANT_TEMP, NULL, 0);
        uart_send_to_gateway(MSG_VOLTAGE, NULL, 0);
        uart_send_to_gateway(MSG_FUEL_LEVEL, NULL, 0);
        uart_send_to_gateway(MSG_ODO, NULL, 0);
        uart_send_to_gateway(MSG_RANGE, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_TIME, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_PAUSE, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_FUEL, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_COST, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_STATUS, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_DIST, NULL, 0);

        CarData_Lock(10);
        int temp_val = data->tempValue;
        int bat_val  = (int)(data->batValue * 10);
        int fuel_val = (int)(data->fuelValue * 10);
        odo_val      = data->odoKm;
        int range_val       = data->rangeValue;
        int trip_time_val   = data->tripValue;
        int trip_pause_val  = data->tripPauseValue;
        int trip_fuel_val   = (int)(data->tripFuelUsed * 10);
        int trip_dist_val   = (int)(data->tripDistanceKm * 10);
        float trip_cost     = data->tripMValue;
        float trip_fuel_litres = data->tripFuelUsed;
        CarData_Unlock();

        lv_subject_set_int(&subject_temp, temp_val);
        lv_subject_set_int(&subject_bat, bat_val);
        lv_subject_set_int(&subject_fuel, fuel_val);
        lv_subject_set_int(&subject_odo, odo_val);
        lv_subject_set_int(&subject_range, range_val);
        lv_subject_set_int(&subject_trip_time, trip_time_val);
        lv_subject_set_int(&subject_trip_pause, trip_pause_val);
        lv_subject_set_int(&subject_trip_fuel, trip_fuel_val);
        lv_subject_set_int(&subject_trip_dist, trip_dist_val);

        show_trip_cost = !show_trip_cost;
        if (show_trip_cost) {
            lv_label_set_text_fmt(ui_DashBoard_Label_TripLitr, "%.2fр", trip_cost);
        } else {
            int lit_int = (int)(trip_fuel_litres * 10);
            lv_label_set_text_fmt(ui_DashBoard_Label_TripLitr, "%d.%dл", lit_int / 10, lit_int % 10);
        }

        if (uart_is_gateway_alive()) {
            lv_obj_remove_flag(ui_DashBoard_Image_BlueRing, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_BlueRing, LV_OBJ_FLAG_HIDDEN);
        }
        if (bat_voltage < BATT_LOW_THRESHOLD || bat_voltage > BATT_HIGH_THRESHOLD) {
            lv_obj_remove_flag(ui_DashBoard_Image_Batallert, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_Batallert, LV_OBJ_FLAG_HIDDEN);
        }
        if (fuel_level < data->fuelRedThreshold) {
            lv_obj_remove_flag(ui_DashBoard_Image_FuelAllert, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_FuelAllert, LV_OBJ_FLAG_HIDDEN);
        }
        if (temp_coolant > TEMP_COLD && temp_coolant <= TEMP_NORMAL) {
            lv_obj_remove_flag(ui_DashBoard_Image_CoolNorm, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_CoolNorm, LV_OBJ_FLAG_HIDDEN);
        }
        if (temp_coolant > TEMP_WARM) {
            lv_obj_remove_flag(ui_DashBoard_Image_CoolHigh, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_CoolHigh, LV_OBJ_FLAG_HIDDEN);
        }
        if (trip_state) {
            lv_obj_remove_flag(ui_DashBoard_Image_TripImg, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_TripImg, LV_OBJ_FLAG_HIDDEN);
        }
        if (speed_kmh > 20) {
            lv_obj_remove_flag(ui_DashBoard_Image_FreeWind, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_DashBoard_Image_FreeWind, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Скринсейвер
    if (!screensaver_active) {
        if (current_rpm < 400) {
            engine_off_seconds++;
        } else {
            engine_off_seconds = 0;
        }
        if (engine_off_seconds >= data->screensaver_timeout_sec) {
            fiona_core_activate_screensaver();
        }
    } else {
        if (current_rpm >= 400) {
            fiona_core_deactivate_screensaver();
        }
    }
}