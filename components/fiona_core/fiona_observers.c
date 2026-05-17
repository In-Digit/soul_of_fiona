/**
 * @file fiona_observers.c
 * @brief Коллбэки LVGL-наблюдателей для виджетов дашборда.
 *
 * ВАЖНО: цвет дуги дросселя теперь определяется стилем вождения (driving_style).
 */

#include "fiona_core.h"
#include "CarData.h"
#include "fiona_brain.h"
#include <stdio.h>
#include <string.h>
#include "uart_protocol.h"

// Внешняя текущая тональность (устанавливается в fiona_background)
extern uint8_t g_current_tone;

// Все необходимые виджеты объявляются как extern (без ui.h)
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
extern lv_obj_t * ui_System_Label_CurrentLight;
extern lv_obj_t * ui_DashBoard_Image_Backgrownd;

// Вспомогательная функция для анимации дуги
static void set_arc_value(void * obj, int32_t new_value) {
    lv_arc_set_value((lv_obj_t *)obj, new_value);
}

// -------------------- Observer'ы --------------------
void speed_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_label_set_text_fmt(ui_DashBoard_Label_SpeedDigit, "%d", val);
    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getSpeedColor(data);
        lv_obj_set_style_text_color(ui_DashBoard_Label_SpeedDigit, lv_color_hex(color), LV_PART_MAIN);
        CarData_Unlock();
    }
}

void rpm_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int new_val = lv_subject_get_int(subj);
    int current_val = lv_arc_get_value(ui_DashBoard_Arc_Taho);

    if (new_val != current_val) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, ui_DashBoard_Arc_Taho);
        lv_anim_set_exec_cb(&a, set_arc_value);
        lv_anim_set_values(&a, current_val, new_val);
        lv_anim_set_duration(&a, 200);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    }

    lv_label_set_text_fmt(ui_DashBoard_Label_TahoLBL, "%d", new_val);

    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getRPMColor(data);
        lv_obj_set_style_arc_color(ui_DashBoard_Arc_Taho, lv_color_hex(color), LV_PART_INDICATOR);
        CarData_Unlock();
    }
}

void bat_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_bar_set_value(ui_DashBoard_Bar_BatBar, val / 10, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_DashBoard_Label_AKB, "%d.%dв", val / 10, val % 10);
    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getBatteryColor(data);
        lv_obj_set_style_bg_color(ui_DashBoard_Bar_BatBar, lv_color_hex(color), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(ui_DashBoard_Label_AKB, lv_color_hex(color), LV_PART_MAIN);
        CarData_Unlock();
    }
}

void fuel_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_bar_set_value(ui_DashBoard_Bar_FuelBar, val / 10, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_DashBoard_Label_FUEL, "%d.%dл", val / 10, val % 10);
    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getFuelColor(data);
        lv_obj_set_style_bg_color(ui_DashBoard_Bar_FuelBar, lv_color_hex(color), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(ui_DashBoard_Label_FUEL, lv_color_hex(color), LV_PART_MAIN);
        CarData_Unlock();
    }
}

void temp_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_bar_set_value(ui_DashBoard_Bar_TempBar, val, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_DashBoard_Label_TempNum, "t-%d°С", val);
    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getTempColor(data);
        lv_obj_set_style_bg_color(ui_DashBoard_Bar_TempBar, lv_color_hex(color), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(ui_DashBoard_Label_TempNum, lv_color_hex(color), LV_PART_MAIN);
        CarData_Unlock();
    }
}

void odo_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_label_set_text_fmt(ui_DashBoard_Label_ODO, "%dкм", val);
}

void range_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_label_set_text_fmt(ui_DashBoard_Label_ODOonFuel, "%dкм", val);
    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getFuelColor(data);
        lv_obj_set_style_text_color(ui_DashBoard_Label_ODOonFuel, lv_color_hex(color), LV_PART_MAIN);
        CarData_Unlock();
    }
}

void lph_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_bar_set_value(ui_DashBoard_Bar_lph, val / 10, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_DashBoard_Label_LPHnum, "%d.%dл/ч", val / 10, val % 10);
    CarData *data = CarData_Get();
    if (data && CarData_Lock(0)) {
        uint32_t color = CarData_getLPHColor(data);
        lv_obj_set_style_bg_color(ui_DashBoard_Bar_lph, lv_color_hex(color), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(ui_DashBoard_Label_LPHnum, lv_color_hex(color), LV_PART_MAIN);
        CarData_Unlock();
    }
}

void trip_time_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    uint32_t sec = (uint32_t)lv_subject_get_int(subj);
    char buf[9];
    CarData_formatTimeHMS(sec, buf, sizeof(buf));
    lv_label_set_text(ui_DashBoard_Label_TripTime, buf);
}

void trip_pause_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    uint32_t sec = (uint32_t)lv_subject_get_int(subj);
    char buf[9];
    CarData_formatTimeHMS(sec, buf, sizeof(buf));
    lv_label_set_text(ui_DashBoard_Label_TripTimePause, buf);
}

void trip_dist_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_label_set_text_fmt(ui_DashBoard_Label_TripNum, "%d.%dкм", val / 10, val % 10);
}

static void clock_color_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_obj_set_style_text_color(ui_DashBoard_Label_Time, lv_color_hex(val == 1 ? 0xFF0000 : 0x9EEFFC), LV_PART_MAIN);
}

static void dash_message_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    const char *msg = (const char*)lv_subject_get_pointer(subj);
    if (msg == NULL || msg[0] == '\0') {
        lv_label_set_text(ui_DashBoard_Label_EditionString, "");
        return;
    }
    lv_label_set_text(ui_DashBoard_Label_EditionString, msg);
    CarData *data = CarData_Get();
    if (data) {
        uint32_t color = data->color_tone_neutral;
        if (g_current_tone == 1) color = data->color_tone_funny;
        else if (g_current_tone == 2) color = data->color_tone_serious;
        lv_obj_set_style_text_color(ui_DashBoard_Label_EditionString, lv_color_hex(color), LV_PART_MAIN);
    }
}

static void adc_light_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);
    lv_label_set_text_fmt(ui_System_Label_CurrentLight, "Освещённость: %d", val);
}

/* Обновлённый наблюдатель дросселя:
 * значение дуги и прозрачность – по throttlePos,
 * цвет дуги – по стилю вождения (driving_style) из FionaState.
 */
void throttle_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);       // положение дросселя 0-100
    uint8_t opa = 64 + (uint8_t)((val * (192 - 32)) / 100);
    lv_arc_set_value(ui_DashBoard_Arc_Drossel, val);
    lv_obj_set_style_arc_opa(ui_DashBoard_Arc_Drossel, opa, LV_PART_INDICATOR);
    lv_label_set_text_fmt(ui_DashBoard_Label_GazBL, "%d%%", val);

    // Определяем цвет по стилю
    uint32_t color = 0x2196F3;   // исходный синий, если стиль не определён
    FionaState *state = fiona_brain_get_state();
    if (state) {
        switch (state->driving_style) {
            case 1: color = 0x00FF00; break;   // зелёный – спокойный
            case 2: color = 0xFFBF00; break;   // жёлтый – агрессивный
            case 3: color = 0xFF3333; break;   // красный – спортивный
            default: break;
        }
    }
    lv_obj_set_style_arc_color(ui_DashBoard_Arc_Drossel, lv_color_hex(color), LV_PART_INDICATOR);
}

void accel_observer_cb(lv_observer_t * obs, lv_subject_t * subj) {
    int val = lv_subject_get_int(subj);   // сотые доли g
    lv_label_set_text_fmt(ui_DashBoard_Label_AxelBL, "%.1f g", val / 100.0f);
    lv_arc_set_value(ui_DashBoard_Arc_Uskorenie, val);
    lv_obj_set_style_arc_opa(ui_DashBoard_Arc_Uskorenie, 96, LV_PART_INDICATOR);
}

/* -------------------------------------------------------------------------- */
/* Подписка всех наблюдателей                                                 */
/* -------------------------------------------------------------------------- */
void fiona_observers_subscribe_all(void) {
    lv_subject_add_observer(&subject_speed, speed_observer_cb, NULL);
    lv_subject_add_observer(&subject_rpm, rpm_observer_cb, NULL);
    lv_subject_add_observer(&subject_bat, bat_observer_cb, NULL);
    lv_subject_add_observer(&subject_fuel, fuel_observer_cb, NULL);
    lv_subject_add_observer(&subject_temp, temp_observer_cb, NULL);
    lv_subject_add_observer(&subject_odo, odo_observer_cb, NULL);
    lv_subject_add_observer(&subject_range, range_observer_cb, NULL);
    lv_subject_add_observer(&subject_lph, lph_observer_cb, NULL);
    lv_subject_add_observer(&subject_trip_time, trip_time_observer_cb, NULL);
    lv_subject_add_observer(&subject_trip_pause, trip_pause_observer_cb, NULL);
    lv_subject_add_observer(&subject_trip_fuel, NULL, NULL);   // не используется отдельно
    lv_subject_add_observer(&subject_trip_dist, trip_dist_observer_cb, NULL);
    lv_subject_add_observer(&subject_clock_color, clock_color_observer_cb, NULL);
    lv_subject_add_observer(&subject_dash_message, dash_message_observer_cb, NULL);
    lv_subject_add_observer(&subject_adc_light, adc_light_observer_cb, NULL);
    lv_subject_add_observer(&subject_throttle, throttle_observer_cb, NULL);
    lv_subject_add_observer(&subject_accel, accel_observer_cb, NULL);
}

/* -------------------------------------------------------------------------- */
/* Начальная инициализация виджетов                                           */
/* -------------------------------------------------------------------------- */
void fiona_observers_init_widgets(void) {
    lv_bar_set_orientation(ui_DashBoard_Bar_BatBar, LV_BAR_ORIENTATION_VERTICAL);
    lv_bar_set_orientation(ui_DashBoard_Bar_FuelBar, LV_BAR_ORIENTATION_VERTICAL);

    lv_arc_set_value(ui_DashBoard_Arc_Drossel, 0);
    lv_obj_set_style_arc_opa(ui_DashBoard_Arc_Drossel, 64, LV_PART_INDICATOR);
    lv_label_set_text(ui_DashBoard_Label_GazBL, "0%");

    lv_arc_set_range(ui_DashBoard_Arc_Uskorenie, -300, 300);
    lv_arc_set_value(ui_DashBoard_Arc_Uskorenie, 0);
    lv_obj_set_style_arc_opa(ui_DashBoard_Arc_Uskorenie, 32, LV_PART_MAIN);
    lv_label_set_text(ui_DashBoard_Label_AxelBL, "0.0 g");

    lv_obj_add_flag(ui_DashBoard_Image_BlueRing, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_Batallert, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_FuelAllert, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_CoolNorm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_CoolHigh, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_TripImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_FreeWind, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ConditionHot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ConditionCold, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_RFirstVent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_RFirstVentAlert, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_RSecondVent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_RSecondVentAlert, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_RSecondVentAlert1, LV_OBJ_FLAG_HIDDEN);

    extern bool uart_send_to_gateway(uint8_t msg_id, const uint8_t *payload, uint8_t len);
    uart_send_to_gateway(MSG_REQ_TIME, NULL, 0);
}