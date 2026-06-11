/**
 * @file fiona_background.c
 * @brief Фоновые таймеры (хирургическая версия с закомментированными картинками)
 *
 * Вентиляторы и климат закомментированы ПОСТРОЧНО.
 * Коты и значки связи активны. Субъекты обновляются всегда.
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
#include "esp_adc/adc_oneshot.h"
#include "ui.h"

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
extern lv_obj_t * ui_DashBoard_Image_ImageCyan;
extern lv_obj_t * ui_DashBoard_Image_ImageGreen;
extern lv_obj_t * ui_DashBoard_Image_ImageYellow;
extern lv_obj_t * ui_DashBoard_Image_ImageRed;
extern lv_obj_t * ui_DashBoard_Image_ImageSpeed;
extern lv_obj_t * ui_DashBoard_Label_CondShim;
extern lv_obj_t * ui_DashBoard_Label_VFirstShim;
extern lv_obj_t * ui_DashBoard_Label_VSecondShim;
extern lv_obj_t * ui_DashBoard_Image_WiFi;
extern lv_obj_t * ui_DashBoard_Image_GPS;
extern lv_obj_t * ui_DashBoard_Image_ServerOn;

extern lv_timer_t *poll_timer;
extern lv_timer_t *clock_timer;
extern bool screensaver_active;

uint8_t g_current_tone = 0;
bool calibration_active = false;
static bool red_alert_active = false;

static float trip_max_pos_accel = 0.0f;
static float trip_max_neg_accel = 0.0f;

static uint32_t last_gw_check_time = 0;
static uint32_t last_ard_check_time = 0;
static bool gw_cached_alive = false;

static uint32_t last_style_request_time = 0;

static uint32_t stop_history[8] = {0};
static int stop_history_idx = 0;

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static float smooth_light = 0.0f;
static uint8_t last_set_brightness = 255;

static void fast_timer_cb(lv_timer_t *timer);
static void clock_timer_cb(lv_timer_t *timer);
static void update_stop_history(uint32_t now);
static void format_float_1(char *buf, size_t size, float value);

void fiona_background_init_timers(void) {
    if (clock_timer == NULL) clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    if (poll_timer == NULL)   poll_timer   = lv_timer_create(fast_timer_cb, 200, NULL);

    if (adc1_handle == NULL) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1, .clk_src = ADC_DIGI_CLK_SRC_DEFAULT, .ulp_mode = ADC_ULP_MODE_DISABLE };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
        adc_oneshot_chan_cfg_t chan_cfg = { .bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12 };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &chan_cfg));
    }
}

static void update_stop_history(uint32_t now) {
    stop_history[stop_history_idx] = now;
    stop_history_idx = (stop_history_idx + 1) % 8;
}

static void format_float_1(char *buf, size_t size, float value) {
    int int_part = (int)value;
    int frac_part = (int)((value - int_part) * 10.0f + 0.5f);
    if (frac_part < 0) frac_part = -frac_part;
    if (frac_part >= 10) { frac_part = 0; int_part++; }
    snprintf(buf, size, "%d.%d", int_part, frac_part);
}

/* ==================== БЫСТРЫЙ ТАЙМЕР ==================== */
static void fast_timer_cb(lv_timer_t *timer) {
    int light_raw = 0; adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &light_raw);
    smooth_light = smooth_light + 0.2f * ((float)light_raw - smooth_light);
    uint8_t max_brightness = 80, min_br = 5, dark_thr = 20, bright_thr = 80, new_duty;
    if (dark_thr < bright_thr) {
        if (smooth_light <= (float)dark_thr) new_duty = min_br;
        else if (smooth_light >= (float)bright_thr) new_duty = max_brightness;
        else { float norm = (smooth_light - (float)dark_thr) / (float)(bright_thr - dark_thr); new_duty = min_br + (uint8_t)((max_brightness - min_br) * powf(norm, 0.5f)); }
    } else { float norm = smooth_light / 4095.0f; new_duty = min_br + (uint8_t)((max_brightness - min_br) * powf(norm, 0.5f)); }
    if (new_duty > 100) new_duty = 100;
    if (new_duty < min_br) new_duty = min_br;
    if (new_duty != last_set_brightness) { bsp_display_brightness_set(new_duty); last_set_brightness = new_duty; }

    CarData *data = CarData_Get();
    if (!data) return;

    int speed_val, rpm_val, lph_val, throttle_val;
    float accel_x;
    CarData_Lock(10);
    speed_val = data->speedValue;
    rpm_val = data->rpmValue;
    lph_val = (int)(data->lphValue * 10);
    throttle_val = (int)data->throttlePos;
    accel_x = data->accel_x;
    CarData_Unlock();

    float accel_g = accel_x / 9.81f;
    if (accel_g > 0) { if (accel_g > trip_max_pos_accel) trip_max_pos_accel = accel_g; int accel_dg = (trip_max_pos_accel > 0.01f) ? (int)(accel_g * 300.0f / trip_max_pos_accel) : (int)(accel_g * 3000); if (accel_dg > 300) accel_dg = 300; lv_subject_set_int(&subject_accel, accel_dg); }
    else if (accel_g < 0) { float neg = -accel_g; if (neg > trip_max_neg_accel) trip_max_neg_accel = neg; int accel_dg = (trip_max_neg_accel > 0.01f) ? (int)(accel_g * 300.0f / trip_max_neg_accel) : (int)(accel_g * 3000); if (accel_dg < -300) accel_dg = -300; lv_subject_set_int(&subject_accel, accel_dg); }
    else lv_subject_set_int(&subject_accel, 0);

    lv_subject_set_int(&subject_speed, speed_val);
    lv_subject_set_int(&subject_rpm, rpm_val);
    lv_subject_set_int(&subject_lph, lph_val);
    lv_subject_set_int(&subject_throttle, throttle_val);
}

/* ==================== СЕКУНДНЫЙ ТАЙМЕР (ХИРУРГИЧЕСКАЯ ВЕРСИЯ) ==================== */
static void clock_timer_cb(lv_timer_t *timer) {
    struct timeval tv; gettimeofday(&tv, NULL); struct tm *timeinfo = localtime(&tv.tv_sec); char buf[64]; uint32_t now = (uint32_t)tv.tv_sec;
    lv_obj_t *act_scr = lv_scr_act(); bool dash_active = (ui_Screen_DashBoard && act_scr == ui_Screen_DashBoard);
    CarData *data = CarData_Get(); if (!data) return;

    /* === ЧТЕНИЕ ДАННЫХ ИЗ CarData === */
    CarData_Lock(10);
    data->systemTime = now;
    if (data->rpmValue > 400) data->last_valid_coolant_temp = data->tempValue;
    FionaState *state = fiona_brain_get_state();
    if (state) { static bool was_engine_off = true; if (data->rpmValue > 400) { if (was_engine_off) { trip_max_pos_accel = 0; trip_max_neg_accel = 0; was_engine_off = false; } if (state->engine_start_time == 0) state->engine_start_time = now; state->trip_duration_sec = now - state->engine_start_time; } else { state->engine_start_time = 0; state->trip_duration_sec = 0; state->long_trip = false; state->very_long_trip = false; was_engine_off = true; } if (data->speedValue == 0) update_stop_history(now); }
    bool trip_force = data->trip_force_active; int tempValue = data->tempValue; int rpmValue = data->rpmValue; float lphValue = data->lphValue;
    float arduino_coolant_temp = data->arduino_coolant_temp; uint8_t arduino_fan_mode = data->arduino_fan_mode; bool arduino_mode_from_screen = data->arduino_mode_from_screen;
    uint8_t fan1 = data->fanCurrentPWM1; uint8_t fan2 = data->fanCurrentPWM2; bool arduino_auto = !data->arduino_mode_from_screen;
    float cabin_temp = data->cabin_temp; float target_temp = data->climate_target_temp;
    float batValue = data->batValue; float fuelValue = data->fuelValue; int rangeValue = data->rangeValue;
    bool uartArduinoAlive = data->uartArduinoAlive;
    int speed_kmh = data->speedValue; uint8_t heater_pwm = data->heater_pwm;
    int current_rpm = data->rpmValue;
    bool wifi_connected = data->wifiConnected;
    bool gps_valid = data->gps_valid;
    CarData_Unlock();

    /* === ПРОВЕРКА СВЯЗИ === */
    if (now - last_gw_check_time >= 5) { gw_cached_alive = uart_is_gateway_alive(); last_gw_check_time = now; }

    /* === ВРЕМЯ === */
    if (dash_active) { strftime(buf, sizeof(buf), "%H:%M:%S", timeinfo); lv_label_set_text(ui_DashBoard_Label_Time, buf); lv_obj_set_style_text_color(ui_DashBoard_Label_Time, lv_color_hex(trip_force ? 0x00FF00 : 0x9EEFFC), LV_PART_MAIN); }

    /* === СПЛЕШ-СКРИН === */
    if (screensaver_active || act_scr == ui_Screen_SplashScreen) { const char *days[] = {"Воскресенье","Понедельник","Вторник","Среда","Четверг","Пятница","Суббота"}; strftime(buf, sizeof(buf), "%d.%m.%Y года\n%H:%M:%S\n", timeinfo); strcat(buf, days[timeinfo->tm_wday]); lv_label_set_text(ui_SplashScreen_Label_Clock, buf); }

    /* === ЗАПРОС СТИЛЯ === */
    if (gw_cached_alive && (now - last_style_request_time >= 5)) { uart_send_to_gateway(MSG_REQ_DRIVING_STYLE, NULL, 0); last_style_request_time = now; }

    /* ========== СУБЪЕКТЫ (ВСЕГДА при активном дашборде) ========== */
    if (dash_active) {
        CarData_Lock(10);
        int bat_val = (int)(data->batValue * 10); int fuel_val = (int)(data->fuelValue * 10); uint32_t odo_val = data->odoKm;
        int range_val = data->rangeValue; int trip_time_val = data->tripValue; int trip_pause_val = data->tripPauseValue;
        int trip_fuel_val = (int)(data->tripFuelUsed * 10); int trip_dist_val = (int)(data->tripDistanceKm * 10);
        CarData_Unlock();

        lv_subject_set_int(&subject_temp, tempValue); lv_subject_set_int(&subject_bat, bat_val); lv_subject_set_int(&subject_fuel, fuel_val);
        lv_subject_set_int(&subject_odo, odo_val); lv_subject_set_int(&subject_range, range_val); lv_subject_set_int(&subject_trip_time, trip_time_val);
        lv_subject_set_int(&subject_trip_pause, trip_pause_val); lv_subject_set_int(&subject_trip_fuel, trip_fuel_val); lv_subject_set_int(&subject_trip_dist, trip_dist_val);
    }

    /* ========== ВИДЖЕТЫ, ЗАВИСЯЩИЕ ОТ ДАННЫХ ШЛЮЗА ========== */
    if (dash_active && gw_cached_alive) {
        /* --- КОТЫ (АКТИВНЫ) --- */
        bool red_condition = (tempValue > 112 && rpmValue > 400 && lphValue > 12) || (tempValue < 90 && lphValue > 12);
        lv_obj_add_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
        if (red_condition) {
            if (lv_obj_has_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN);
            red_alert_active = true;
        } else {
            red_alert_active = false;
            if (state) {
                uint8_t style = state->manual_style ? state->manual_style : state->driving_style;
                if (style == 1 && lv_obj_has_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
                else if (style == 2 && lv_obj_has_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN);
                else if (style == 3 && lv_obj_has_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN);
                if (state->manual_style != 0 && lv_obj_has_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
            }
        }

        /* --- ВЕНТИЛЯТОРЫ (ЗАКОММЕНТИРОВАНЫ ПОСТРОЧНО) --- */
         lv_obj_add_flag(ui_DashBoard_Image_RFirstVent, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_DashBoard_Image_RSecondVent, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_DashBoard_Image_RFirstVentAlert, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_DashBoard_Image_RSecondVentAlert, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_DashBoard_Image_RSecondVentAlert1, LV_OBJ_FLAG_HIDDEN);
        // if (fan1 > 0) {
        //     if (lv_obj_has_flag(ui_DashBoard_Image_RFirstVent, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_RFirstVent, LV_OBJ_FLAG_HIDDEN);
        //     if (fan1 > 191 && lv_obj_has_flag(ui_DashBoard_Image_RFirstVentAlert, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_RFirstVentAlert, LV_OBJ_FLAG_HIDDEN);
        // }
        // if (fan2 > 0) {
        //     if (lv_obj_has_flag(ui_DashBoard_Image_RSecondVent, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_RSecondVent, LV_OBJ_FLAG_HIDDEN);
        //     if (fan2 > 191 && lv_obj_has_flag(ui_DashBoard_Image_RSecondVentAlert, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_RSecondVentAlert, LV_OBJ_FLAG_HIDDEN);
        // }
        // if (arduino_auto && lv_obj_has_flag(ui_DashBoard_Image_RSecondVentAlert1, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_RSecondVentAlert1, LV_OBJ_FLAG_HIDDEN);

        /* --- КЛИМАТ (ЗАКОММЕНТИРОВАН ПОСТРОЧНО) --- */
        // lv_obj_add_flag(ui_DashBoard_Image_FreeWind, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_DashBoard_Image_ConditionHot, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_DashBoard_Image_ConditionCold, LV_OBJ_FLAG_HIDDEN);
        // if (speed_kmh > 20 && heater_pwm == 0 && lv_obj_has_flag(ui_DashBoard_Image_FreeWind, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_FreeWind, LV_OBJ_FLAG_HIDDEN);
        // float diff = cabin_temp - target_temp;
        // if (diff > 3.0f && lv_obj_has_flag(ui_DashBoard_Image_ConditionCold, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ConditionCold, LV_OBJ_FLAG_HIDDEN);
        // else if (diff < -5.0f && lv_obj_has_flag(ui_DashBoard_Image_ConditionHot, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_ConditionHot, LV_OBJ_FLAG_HIDDEN);

        /* --- МЕТКИ (АКТИВНЫ) --- */
        if (ui_DashBoard_Label_VFirstShim) {
            if (arduino_coolant_temp > 0 || fan1 > 0) { char temp_buf[16]; format_float_1(temp_buf, sizeof(temp_buf), arduino_coolant_temp); lv_label_set_text_fmt(ui_DashBoard_Label_VFirstShim, "%s C", temp_buf); if (lv_obj_has_flag(ui_DashBoard_Label_VFirstShim, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Label_VFirstShim, LV_OBJ_FLAG_HIDDEN); }
            else lv_obj_add_flag(ui_DashBoard_Label_VFirstShim, LV_OBJ_FLAG_HIDDEN);
        }
        if (ui_DashBoard_Label_VSecondShim) {
            const char *m = (arduino_fan_mode==1)?"Nrm":(arduino_fan_mode==2)?"Hwy":"Cty"; lv_label_set_text(ui_DashBoard_Label_VSecondShim, m);
            if (lv_obj_has_flag(ui_DashBoard_Label_VSecondShim, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Label_VSecondShim, LV_OBJ_FLAG_HIDDEN);
        }
        if (ui_DashBoard_Label_CondShim) { char temp_buf[16]; format_float_1(temp_buf, sizeof(temp_buf), cabin_temp); lv_label_set_text_fmt(ui_DashBoard_Label_CondShim, "%s C", temp_buf); if (lv_obj_has_flag(ui_DashBoard_Label_CondShim, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Label_CondShim, LV_OBJ_FLAG_HIDDEN); }

        /* --- СТАТУСНАЯ СТРОКА (АКТИВНА) --- */
        if (!calibration_active) {
            char line[512] = {0}; char temp[128] = {0};
            if (state && state->trip_duration_sec > 0) { uint16_t h = state->trip_duration_sec / 3600; uint8_t m = (state->trip_duration_sec % 3600) / 60; snprintf(temp, sizeof(temp), "Мы в пути %u ч %u мин. ", h, m); strcat(line, temp); }
            char tbuf[16]; format_float_1(tbuf, sizeof(tbuf), cabin_temp);
            if (cabin_temp > target_temp + 3.0f) snprintf(temp, sizeof(temp), "В салоне жарковато (%s°C). ", tbuf);
            else if (cabin_temp < target_temp - 5.0f) snprintf(temp, sizeof(temp), "В салоне прохладно (%s°C). ", tbuf);
            else snprintf(temp, sizeof(temp), "В салоне комфортно (%s°C). ", tbuf);
            strcat(line, temp);
            strcat(line, uartArduinoAlive ? (arduino_mode_from_screen ? "Охлаждение под контролем. " : "Охлаждение работает самостоятельно. ") : "Охлаждение недоступно. ");
            if (state) {
                const char *style_str = "Не определён";
                if (state->manual_style) { style_str = (state->manual_style==1)?"Спокойный":(state->manual_style==2)?"Агрессивный":"Спорт"; snprintf(temp, sizeof(temp), "Стиль (ручной): %s. ", style_str); }
                else { style_str = (state->driving_style==1)?"Спокойный":(state->driving_style==2)?"Агрессивный":(state->driving_style==3)?"Спорт":"Не определён"; snprintf(temp, sizeof(temp), "Стиль: %s. ", style_str); }
                strcat(line, temp);
                const char *mode_str = (state->driving_mode==1)?"Город":(state->driving_mode==2)?"Пробка":(state->driving_mode==3)?"Трасса":"Спокойно";
                snprintf(temp, sizeof(temp), "Режим: %s. ", mode_str); strcat(line, temp);
            }
            char fbuf[16]; format_float_1(fbuf, sizeof(fbuf), fuelValue);
            snprintf(temp, sizeof(temp), "Остаток в баке %s л, ", fbuf); strcat(line, temp);
            if (rangeValue > 0) snprintf(temp, sizeof(temp), "хватит на %d км. ", rangeValue); else snprintf(temp, sizeof(temp), "расчёт хода недоступен. "); strcat(line, temp);
            bool all_ok = true;
            if (batValue < BATT_LOW_THRESHOLD || batValue > BATT_HIGH_THRESHOLD) { strcat(line, "Внимание: проблема с напряжением АКБ! "); all_ok = false; }
            if (tempValue > TEMP_HOT) { strcat(line, "Перегрев двигателя! "); all_ok = false; }
            if (fuelValue < FUEL_RED_THRESHOLD) { strcat(line, "Критический остаток топлива! "); all_ok = false; }
            if (all_ok) strcat(line, "Все системы в норме.");
            if (ui_DashBoard_Label_FionaSpeachLabelDash) lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, line);
        }

        /* --- ИНДИКАТОРЫ СВЯЗИ (АКТИВНЫ) --- */
        if (gw_cached_alive) { if (lv_obj_has_flag(ui_DashBoard_Image_BlueRing, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_BlueRing, LV_OBJ_FLAG_HIDDEN); }
        else lv_obj_add_flag(ui_DashBoard_Image_BlueRing, LV_OBJ_FLAG_HIDDEN);
        if (wifi_connected) { if (lv_obj_has_flag(ui_DashBoard_Image_WiFi, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_WiFi, LV_OBJ_FLAG_HIDDEN); }
        else lv_obj_add_flag(ui_DashBoard_Image_WiFi, LV_OBJ_FLAG_HIDDEN);
        if (gps_valid) { if (lv_obj_has_flag(ui_DashBoard_Image_GPS, LV_OBJ_FLAG_HIDDEN)) lv_obj_remove_flag(ui_DashBoard_Image_GPS, LV_OBJ_FLAG_HIDDEN); }
        else lv_obj_add_flag(ui_DashBoard_Image_GPS, LV_OBJ_FLAG_HIDDEN);
    }

    /* === АВТОВОЗВРАТ === */
    if (!screensaver_active && act_scr != ui_Screen_DashBoard && act_scr != ui_Screen_SplashScreen) {
        inactivity_seconds++;
        if (inactivity_seconds >= data->screensaver_timeout_sec) { inactivity_seconds = 0; _ui_screen_change(&ui_Screen_DashBoard, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_DashBoard_screen_init); }
    } else inactivity_seconds = 0;

    /* === СКРИНСЕЙВЕР === */
    if (!screensaver_active) { if (current_rpm < 400) engine_off_seconds++; else engine_off_seconds = 0; if (engine_off_seconds >= data->screensaver_timeout_sec) fiona_core_activate_screensaver(); }
    else if (current_rpm >= 400) fiona_core_deactivate_screensaver();
}