/**
 * @file ui_events.c
 * @brief Обработчики событий GUI.
 *
 * Включает обработку климата (слайдер температуры -20..+20, ШИМ печки),
 * калибровку вентиляторов/печки (через выпадающий список), заправку, время.
 * Адаптировано под новую логику Arduino и климат-контроля.
 */

#include "ui.h"
#include "fiona_core.h"
#include "system_actions.h"
#include "ui_debug_helpers.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "protocol.h"
#include "sd_utils.h"
#include "fiona_brain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"

// Внешние переменные и функции из ядра
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
extern lv_timer_t * poll_timer;
extern lv_timer_t * clock_timer;
extern bool screensaver_active;

// ---------- Переменные для ручного выбора стиля ----------
static uint8_t selected_style = 0;
static lv_timer_t * style_confirm_timer = NULL;

// ---------- Переменные для калибровки ----------
static int8_t calib_selected_index = -1;
static uint8_t calib_current_value = 0;
static bool calib_active = false;

// Прототипы
static void style_confirm_timer_cb(lv_timer_t * t);
static int get_calib_device(void);
static int get_point_type(void);
static uint8_t get_calib_cmd(int point_type, int device);
static void apply_calib_save(void);

/* ---------------------------------------------------------------------------
 * Вспомогательные функции калибровки
 * --------------------------------------------------------------------------- */
static int get_calib_device(void) {
    if (calib_selected_index < 0 || calib_selected_index > 11) return -1;
    if (calib_selected_index < 8) return 0; // В1 или В2 -> Arduino
    return 1; // ВП -> печка (шлюз)
}

static int get_point_type(void) {
    if (calib_selected_index < 0) return 0;
    return calib_selected_index % 4; // 0=старт, 1=стоп, 2=ВЧ, 3=аэродинамика
}

static uint8_t get_calib_cmd(int point_type, int device) {
    if (device == 0) { // Arduino
        switch (point_type) {
            case 0: return MSG_FAN_CALIB_START_POINT;
            case 1: return MSG_FAN_CALIB_STOP_POINT;
            case 2: return MSG_FAN_CALIB_NOISE_LOW;
            case 3: return MSG_FAN_CALIB_NOISE_HIGH;
        }
    } else { // Печка
        switch (point_type) {
            case 0: return MSG_HEATER_CALIB_START_POINT;
            case 1: return MSG_HEATER_CALIB_STOP_POINT;
            case 2: return MSG_HEATER_CALIB_NOISE_LOW;
            case 3: return MSG_HEATER_CALIB_NOISE_HIGH;
        }
    }
    return 0;
}

static void apply_calib_save(void) {
    int device = get_calib_device();
    if (device == 0) {
        uart_send_to_arduino(MSG_FAN_CALIB_SAVE, NULL, 0);
    } else if (device == 1) {
        uart_send_to_gateway(MSG_HEATER_CALIB_SAVE, NULL, 0);
    }
    lv_obj_add_flag(ui_System_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_System_Label_FanSaved, LV_OBJ_FLAG_HIDDEN);
    calib_active = false;
}

/* ---------------------------------------------------------------------------
 * Обработчики событий дашборда и системных действий
 * --------------------------------------------------------------------------- */
void DashOnFocused(lv_event_t * e) {}
void OnDefocused(lv_event_t * e) {}

void DashboardOnLoad(lv_event_t * e) {
    fiona_core_dashboard_on_load();
}

void GetInternetTime(lv_event_t * e) {
    fiona_core_request_internet_time();
    uart_send_to_gateway(MSG_REQ_TIME, NULL, 0);
}

void OnTripClick(lv_event_t * e) {
    uart_send_to_gateway(MSG_TRIP_TOGGLE, NULL, 0);
}

// ------------------- Заправка -------------------
void OnRefuelFocused(lv_event_t * e) {
    CarData *data = CarData_Get();
    if (!data) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", data->fuelValue);
    lv_textarea_set_text(ui_System_Textarea_TACurrentFuel, buf);
    uint32_t miles = (uint32_t)(data->odoKm * 0.621371f);
    snprintf(buf, sizeof(buf), "%u", miles);
    lv_textarea_set_text(ui_System_Textarea_TAODO, buf);
    snprintf(buf, sizeof(buf), "%.2f", data->fuelPrice);
    lv_textarea_set_text(ui_System_Textarea_TAPrise, buf);
    lv_textarea_set_text(ui_System_Textarea_TAFuelToAdd, "");
    lv_obj_add_flag(ui_System_Label_SaveLBtn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_state(ui_System_Switch_SwitchFullTank, LV_STATE_CHECKED);
}

void OnRefueDefFocused(lv_event_t * e) {}
void OnCalibrateClicked(lv_event_t * e) {}

void SaveChangeBtnClick(lv_event_t * e) {
    CarData *data = CarData_Get();
    if (!data) return;
    const char *fuel_add_text = lv_textarea_get_text(ui_System_Textarea_TAFuelToAdd);
    const char *fuel_curr_text = lv_textarea_get_text(ui_System_Textarea_TACurrentFuel);
    const char *odo_text = lv_textarea_get_text(ui_System_Textarea_TAODO);
    const char *price_text = lv_textarea_get_text(ui_System_Textarea_TAPrise);
    bool switch_full_tank = lv_obj_has_state(ui_System_Switch_SwitchFullTank, LV_STATE_CHECKED);

    bool odo_filled = (odo_text[0] != '\0');
    bool fuel_add_filled = (fuel_add_text[0] != '\0');
    bool fuel_curr_filled = (fuel_curr_text[0] != '\0');
    bool price_filled = (price_text[0] != '\0');

    bool saved = false;

    if (fuel_curr_filled && odo_filled) {
        float fuel_current = atof(fuel_curr_text);
        uint32_t miles = atoi(odo_text);
        uint32_t odo_km = (uint32_t)(miles * 1.60934f);
        CarData_Lock(100);
        data->fuelValue = fuel_current;
        data->odoKm = odo_km;
        CarData_Unlock();
        uart_send_set_fuel_level(fuel_current);
        uart_send_set_odo(odo_km);
        if (price_filled) {
            float price = atof(price_text);
            CarData_Lock(100);
            data->fuelPrice = price;
            CarData_Unlock();
        }
        saved = true;
    }

    if (fuel_add_filled && price_filled) {
        float fuel_add = atof(fuel_add_text);
        float price = atof(price_text);
        uart_send_refuel_data(fuel_add, price);
        CarData_Lock(100);
        data->fuelPrice = price;
        CarData_Unlock();
        RefuelRecord rec;
        rec.timestamp = time(NULL);
        rec.liters = fuel_add;
        rec.price = price;
        if (odo_filled) {
            uint32_t miles = atoi(odo_text);
            rec.odo_km = (uint32_t)(miles * 1.60934f);
        } else {
            rec.odo_km = data->odoKm;
        }
        rec.cost = fuel_add * price;
        rec.calibration = switch_full_tank;
        sd_write_refuel_record(&rec);
        CarData_Lock(100);
        memcpy(&data->lastRefuel, &rec, sizeof(RefuelRecord));
        if (switch_full_tank && odo_filled) {
            uint32_t miles = atoi(odo_text);
            data->odoKm = (uint32_t)(miles * 1.60934f);
            uart_send_set_odo(data->odoKm);
            uart_send_full_tank_flag(1);
        }
        CarData_Unlock();
        saved = true;
    }

    if (saved) {
        lv_obj_add_flag(ui_System_Label_SaveLBtn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_System_Label_Zapravleno, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_System_Textarea_TAFuelToAdd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_System_Label_LitrPrise, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_System_Textarea_TAPrise, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_System_Label_Calibrate, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_System_Switch_SwitchFullTank, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(ui_System_Textarea_TACurrentFuel, "");
        lv_textarea_set_text(ui_System_Textarea_TAODO, "");
        lv_textarea_set_text(ui_System_Textarea_TAFuelToAdd, "");
        lv_textarea_set_text(ui_System_Textarea_TAPrise, "");
        _ui_screen_change(&ui_Screen_DashBoard, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen_DashBoard_screen_init);
    }
}

// ------------------- Настройки времени -------------------
void SetTimeClicked(lv_event_t * e) {
    system_actions_set_time_from_textarea();
}
void SetTimeRClicked(lv_event_t * e) {
    system_actions_request_time();
}

/* ---------------------------------------------------------------------------
 * Вкладка «Настройки»: слайдеры яркости
 * --------------------------------------------------------------------------- */
void SettingFocused(lv_event_t * e) {
    CarData *data = CarData_Get();
    if (!data) return;
    CarData_Lock(10);
    uint8_t brightness = data->backlight_brightness;
    uint8_t threshold_bright = data->light_threshold_bright;
    uint8_t threshold_dark = data->light_threshold_dark;
    CarData_Unlock();
    lv_slider_set_value(ui_System_Slider_SliderBridges, brightness, LV_ANIM_OFF);
    lv_slider_set_value(ui_System_Slider_SliderLight, threshold_bright, LV_ANIM_OFF);
    lv_slider_set_value(ui_System_Slider_SliderDark, threshold_dark, LV_ANIM_OFF);
}
void SettingDeFocused(lv_event_t * e) {}

void OnLightSliderChanged(lv_event_t * e) {
    CarData *data = CarData_Get();
    if (!data) return;
    int val = lv_slider_get_value(ui_System_Slider_SliderBridges);
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    CarData_Lock(10);
    data->backlight_brightness = (uint8_t)val;
    CarData_Unlock();
    lv_label_set_text_fmt(ui_System_Label_LightLbl, "Яркость: %d", val);
}

void OnLightSliderThresholdChanged(lv_event_t * e) {
    CarData *data = CarData_Get();
    if (!data) return;
    int val = lv_slider_get_value(ui_System_Slider_SliderLight);
    if (val < 0) val = 0;
    if (val > 4095) val = 4095;
    CarData_Lock(10);
    data->light_threshold_bright = (uint8_t)val;
    CarData_Unlock();
    lv_label_set_text_fmt(ui_System_Label_LightMarkLbl, "Светло: %d", val);
}

void OnSliderThresholdChanged(lv_event_t * e) {
    CarData *data = CarData_Get();
    if (!data) return;
    int val = lv_slider_get_value(ui_System_Slider_SliderDark);
    if (val < 0) val = 0;
    if (val > 4095) val = 4095;
    CarData_Lock(10);
    data->light_threshold_dark = (uint8_t)val;
    CarData_Unlock();
    lv_label_set_text_fmt(ui_System_Label_DarkMarkLbl, "Темно: %d", val);
}

void SaveSettingChangeBtnClick(lv_event_t * e) {
    fiona_core_save_car_data_to_nvs();
    lv_label_set_text(ui_System_Label_LightLbl, "Сохранено!");
}

/* ---------------------------------------------------------------------------
 * Климат-контроль
 * --------------------------------------------------------------------------- */
void ToHomeClick(lv_event_t * e) {}

void TempChanged(lv_event_t * e) {
    // Слайдер теперь -20..+20 симметричный
    int val = lv_slider_get_value(ui_ClimateControl_Slider_TempSlider);
    float target = 22.0f + (float)val;
    if (target < 10.0f) target = 10.0f;
    if (target > 35.0f) target = 35.0f;
    int16_t temp_x10 = (int16_t)(target * 10.0f);
    uint8_t payload[2] = {
        (uint8_t)(temp_x10 & 0xFF),
        (uint8_t)((temp_x10 >> 8) & 0xFF)
    };
    uart_send_to_gateway(MSG_CLIMATE_SET_TEMP, payload, 2);
    if (lv_obj_has_state(ui_ClimateControl_Switch_TempSwitch, LV_STATE_CHECKED)) {
        lv_obj_remove_state(ui_ClimateControl_Switch_TempSwitch, LV_STATE_CHECKED);
        uint8_t auto_off = 0;
        uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_off, 1);
    }
    // Обновляем метку желаемой температуры
    if (ui_ClimateControl_Label_Temperatura) {
        lv_label_set_text_fmt(ui_ClimateControl_Label_Temperatura, "%.1f°C", target);
    }
}

void AlignChanged(lv_event_t * e) {}

void FlowChanged(lv_event_t * e) {
    uint8_t pwm = (uint8_t)lv_slider_get_value(ui_ClimateControl_Slider_FlowSlider);
    uart_send_to_gateway(MSG_CLIMATE_SET_PWM, &pwm, 1);
    if (lv_obj_has_state(ui_ClimateControl_Switch_FlpwSwitch, LV_STATE_CHECKED)) {
        lv_obj_remove_state(ui_ClimateControl_Switch_FlpwSwitch, LV_STATE_CHECKED);
        uint8_t auto_off = 0;
        uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_off, 1);
    }
    if (ui_ClimateControl_Label_FlowShim) {
        lv_label_set_text_fmt(ui_ClimateControl_Label_FlowShim, "%d%%", (pwm * 100) / 255);
    }
}

void TempAutoChange(lv_event_t * e) {
    uint8_t auto_state = lv_obj_has_state(ui_ClimateControl_Switch_TempSwitch, LV_STATE_CHECKED) ? 1 : 0;
    uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_state, 1);
}

void AlignAutoChange(lv_event_t * e) {}

void FlowAutoChange(lv_event_t * e) {
    uint8_t auto_state = lv_obj_has_state(ui_ClimateControl_Switch_FlpwSwitch, LV_STATE_CHECKED) ? 1 : 0;
    uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_state, 1);
}

/* ---------------------------------------------------------------------------
 * Калибровка вентиляторов/печки
 * --------------------------------------------------------------------------- */
void FanChangeDot(lv_event_t * e) {
    calib_selected_index = lv_dropdown_get_selected(ui_System_Dropdown_DropdownFanDot);
    // Загружаем текущее значение точки из CarData
    CarData *data = CarData_Get();
    if (!data) return;
    int device = get_calib_device();
    int pt = get_point_type();
    if (device == 1) { // печка
        switch (pt) {
            case 0: calib_current_value = data->climateCalibStartPoint; break;
            case 1: calib_current_value = data->climateCalibStopPoint; break;
            case 2: calib_current_value = data->climateCalibNoiseLow; break;
            case 3: calib_current_value = data->climateCalibNoiseHigh; break;
        }
    } else {
        // Для Arduino пока не храним отдельно, можно взять из CarData или 0
        calib_current_value = 0;
    }
    lv_label_set_text_fmt(ui_System_Label_FanCalibLbl, "%d", calib_current_value);
    // Показываем кнопки +/- и скрываем "сохранено"
    lv_obj_add_flag(ui_System_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_System_Label_FanSaved, LV_OBJ_FLAG_HIDDEN);
}

void FanPlusClk(lv_event_t * e) {
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    if (device == 1 && !calib_active) {
        // Запуск интерактивной калибровки печки
        uart_send_to_gateway(MSG_HEATER_CALIB_START, NULL, 0);
        calib_active = true;
    }
    if (calib_current_value < 255) calib_current_value++;
    lv_label_set_text_fmt(ui_System_Label_FanCalibLbl, "%d", calib_current_value);
    if (device == 1) {
        // Отправляем шаг калибровки
        uint8_t step = 1;
        uart_send_to_gateway(MSG_HEATER_CALIB_STEP, &step, 1);
    }
    lv_obj_remove_flag(ui_System_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_System_Label_FanSaved, LV_OBJ_FLAG_HIDDEN);
}

void FanMinusClk(lv_event_t * e) {
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    if (device == 1 && !calib_active) {
        uart_send_to_gateway(MSG_HEATER_CALIB_START, NULL, 0);
        calib_active = true;
    }
    if (calib_current_value > 0) calib_current_value--;
    lv_label_set_text_fmt(ui_System_Label_FanCalibLbl, "%d", calib_current_value);
    if (device == 1) {
        uint8_t step = (uint8_t)-1; // шаг вниз
        uart_send_to_gateway(MSG_HEATER_CALIB_STEP, &step, 1);
    }
    lv_obj_remove_flag(ui_System_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_System_Label_FanSaved, LV_OBJ_FLAG_HIDDEN);
}

void FanSaveApply(lv_event_t * e) {
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    int pt = get_point_type();
    uint8_t cmd = get_calib_cmd(pt, device);
    if (device == 0) {
        // Arduino: сразу отправляем значение и сохраняем
        uart_send_to_arduino(cmd, &calib_current_value, 1);
        uart_send_to_arduino(MSG_FAN_CALIB_SAVE, NULL, 0);
    } else {
        // Печка: фиксируем точку
        uart_send_to_gateway(cmd, &calib_current_value, 1);
        // Если это последняя точка (аэродинамика), завершаем калибровку
        if (pt == 3) {
            apply_calib_save();
        }
    }
    lv_obj_add_flag(ui_System_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_System_Label_FanSaved, LV_OBJ_FLAG_HIDDEN);
    // Обновляем CarData для печки
    if (device == 1) {
        CarData *data = CarData_Get();
        if (data) {
            CarData_Lock(10);
            switch (pt) {
                case 0: data->climateCalibStartPoint = calib_current_value; break;
                case 1: data->climateCalibStopPoint = calib_current_value; break;
                case 2: data->climateCalibNoiseLow = calib_current_value; break;
                case 3: data->climateCalibNoiseHigh = calib_current_value; break;
            }
            CarData_Unlock();
        }
    }
}

/* ---------------------------------------------------------------------------
 * Дебаг-вкладки
 * --------------------------------------------------------------------------- */
void OnDebugArduino(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_debug_update_arduino_tab();
    }
}
void OnDebugESP32(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_debug_update_esp32_tab();
    }
}
void OnDebugScreen(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_debug_update_screen_tab();
    }
}

/* ---------------------------------------------------------------------------
 * Скринсейвер
 * --------------------------------------------------------------------------- */
void OnScreenSaverClk(lv_event_t * e) {
    fiona_core_deactivate_screensaver();
}

/* ---------------------------------------------------------------------------
 * Калибровка IMU
 * --------------------------------------------------------------------------- */
void CalibrateMCUBtnClick(lv_event_t * e) {
    uart_send_to_gateway(MSG_CALIBRATE_ACCEL, NULL, 0);
    lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "Калибровка запущена");
    extern bool calibration_active;
    calibration_active = true;
}

/* ---------------------------------------------------------------------------
 * Ручной выбор стиля (котики)
 * --------------------------------------------------------------------------- */
void RezChange(lv_event_t * e) {
    selected_style = (selected_style + 1) % 4;
    lv_obj_add_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN);
    if (selected_style == 1) {
        lv_obj_remove_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
    } else if (selected_style == 2) {
        lv_obj_remove_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN);
    } else if (selected_style == 3) {
        lv_obj_remove_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
    if (style_confirm_timer) {
        lv_timer_del(style_confirm_timer);
    }
    style_confirm_timer = lv_timer_create(style_confirm_timer_cb, 5000, NULL);
}

static void style_confirm_timer_cb(lv_timer_t * t) {
    FionaState *state = fiona_brain_get_state();
    if (state) {
        state->manual_style = selected_style;
    }
    lv_obj_add_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t);
    style_confirm_timer = NULL;
}

/* ---------------------------------------------------------------------------
 * Пустые обработчики
 * --------------------------------------------------------------------------- */
void ImageAnimOnLoad(lv_event_t * e) {}
void ImageAnimOnUnLoad(lv_event_t * e) {}