/**
 * @file fiona_background.c
 * @brief Фоновые таймеры и периодическая логика ядра.
 *
 * Управляет отображением «котов» (индикаторов стиля) и аварийного красного кота.
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

/* Новые UI-объекты для «котов» */
extern lv_obj_t * ui_DashBoard_Image_ImageCyan;
extern lv_obj_t * ui_DashBoard_Image_ImageGreen;
extern lv_obj_t * ui_DashBoard_Image_ImageYellow;
extern lv_obj_t * ui_DashBoard_Image_ImageRed;
extern lv_obj_t * ui_DashBoard_Image_ImageSpeed;

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

// ADC для фоторезистора (GPIO20)
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// Сглаженное значение освещённости и последняя установленная яркость
static float smooth_light = 0.0f;
static uint8_t last_set_brightness = 255;

// Флаг активности калибровки
bool calibration_active = false;

// Для отслеживания состояния аварийного красного кота
static bool red_alert_active = false;

// Прототипы
static void fast_timer_cb(lv_timer_t *timer);
static void clock_timer_cb(lv_timer_t *timer);
static void update_stop_history(uint32_t now);
static int count_stops_last_5min(uint32_t now);

void fiona_background_init_timers(void) {
    if (clock_timer == NULL) clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    if (poll_timer == NULL)   poll_timer   = lv_timer_create(fast_timer_cb, 100, NULL);

    // Инициализация ADC для фоторезистора (GPIO20) – только один раз
    if (adc1_handle == NULL) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
            .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
        adc_oneshot_chan_cfg_t chan_cfg = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &chan_cfg));
    }
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
    // --- Автояркость по своему фоторезистору (GPIO20) – всегда, даже без шлюза ---
    int light_raw = 0;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &light_raw);

    // Экспоненциальное сглаживание (α = 0.2)
    smooth_light = smooth_light + 0.2f * ((float)light_raw - smooth_light);

    uint8_t max_brightness = 80;
    uint8_t min_br = 5;
    uint8_t dark_thr = 20;
    uint8_t bright_thr = 80;

    uint8_t new_duty;
    if (dark_thr < bright_thr) {
        if (smooth_light <= (float)dark_thr) {
            new_duty = min_br;
        } else if (smooth_light >= (float)bright_thr) {
            new_duty = max_brightness;
        } else {
            float norm = (smooth_light - (float)dark_thr) / (float)(bright_thr - dark_thr);
            float curved = powf(norm, 0.5f);
            new_duty = min_br + (uint8_t)((max_brightness - min_br) * curved);
        }
    } else {
        float norm = smooth_light / 4095.0f;
        float curved = powf(norm, 0.5f);
        new_duty = min_br + (uint8_t)((max_brightness - min_br) * curved);
    }

    if (new_duty > 100) new_duty = 100;
    if (new_duty < min_br) new_duty = min_br;

    if (new_duty != last_set_brightness) {
        bsp_display_brightness_set(new_duty);
        last_set_brightness = new_duty;
    }

    // --- Дальше стандартный код, требующий связи со шлюзом ---
    CarData *data = CarData_Get();
    if (!data || !uart_is_gateway_alive()) {
        return;
    }

    static uint8_t fast_counter = 0;
    fast_counter++;

    if (fast_counter % 5 == 0) {
        uart_send_to_gateway(MSG_RPM, NULL, 0);
        uart_send_to_gateway(MSG_THROTTLE, NULL, 0);
    }
    uart_send_to_gateway(MSG_REQ_ACCEL, NULL, 0);
    if (fast_counter % 2 == 0) {
        uart_send_to_gateway(MSG_SPEED, NULL, 0);
        uart_send_to_gateway(MSG_INST_FUEL, NULL, 0);
    }

    if (fast_counter >= 10) fast_counter = 0;

    int speed_val, rpm_val, lph_val, throttle_val;
    float accel_x;
    CarData_Lock(10);
    speed_val = data->speedValue;
    rpm_val = data->rpmValue;
    lph_val = (int)(data->lphValue * 10);
    throttle_val = (int)data->throttlePos;
    accel_x = data->accel_x;

    // Автоматическое обновление пиковых перегрузок
    float accel_g = accel_x / 9.81f;
    if (accel_g > data->max_pos_accel_g) {
        data->max_pos_accel_g = accel_g;
    }
    if (accel_g < data->max_neg_accel_g) {
        data->max_neg_accel_g = accel_g;
    }
    CarData_Unlock();

    lv_subject_set_int(&subject_speed, speed_val);
    lv_subject_set_int(&subject_rpm, rpm_val);
    lv_subject_set_int(&subject_lph, lph_val);
    lv_subject_set_int(&subject_throttle, throttle_val);

    // Обновление дуги ускорения: переводим в сотые доли g и отправляем в субъект
    int accel_dg = (int)(accel_g * 100.0f);
    if (accel_dg > 300) accel_dg = 300;
    if (accel_dg < -300) accel_dg = -300;
    lv_subject_set_int(&subject_accel, accel_dg);
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

    // --- Опрос статуса калибровки IMU (читаем из CarData) ---
    if (calibration_active) {
        uart_send_to_gateway(MSG_REQ_CALIB_STATUS, NULL, 0);
        switch (data->calib_status) {
            case 0x01:
                lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "Калибровка: не двигайтесь");
                break;
            case 0x02:
                lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "Разгонитесь, прокатитесь и затормозите");
                break;
            case 0x03:
                lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "Калибровка завершена");
                calibration_active = false;
                break;
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
                lv_label_set_text(ui_DashBoard_Label_FionaSpeachLabelDash, "Ошибка калибровки. Повторите.");
                calibration_active = false;
                break;
            default:
                break;
        }
    }

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
            // Осмысленная реплика – в бегущую строку
            lv_label_set_text(ui_DashBoard_Label_EditionString, phrase.text);
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
    }

    // ============ УПРАВЛЕНИЕ «КОТАМИ» ============
    bool red_condition = false;
    if (data) {
        float temp = data->tempValue;
        float lph  = data->lphValue;
        int   rpm  = data->rpmValue;
        // Условие аварийного красного кота
        if ((temp > 112 && rpm > 400 && lph > 12) || (temp < 90 && lph > 12)) {
            red_condition = true;
        }
    }

    // Всегда скрываем котов и ImageSpeed по умолчанию, потом покажем нужные
    lv_obj_add_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);

    if (red_condition) {
        // Показываем только красного кота
        lv_obj_remove_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN);
        red_alert_active = true;
    } else {
        red_alert_active = false;
        // Нормальное отображение
        if (state) {
            // Если есть ручной выбор, используем его; иначе берём автоопределение
            uint8_t style = state->manual_style ? state->manual_style : state->driving_style;
            lv_obj_t *cat_to_show = NULL;
            if (style == 1) cat_to_show = ui_DashBoard_Image_ImageGreen;   // спокойный
            else if (style == 2) cat_to_show = ui_DashBoard_Image_ImageYellow; // агрессивный
            else if (style == 3) cat_to_show = ui_DashBoard_Image_ImageCyan;   // спорт
            // style == 0 – не показываем ничего

            if (cat_to_show) {
                lv_obj_remove_flag(cat_to_show, LV_OBJ_FLAG_HIDDEN);
            }

            // Индикатор ручного режима
            if (state->manual_style != 0) {
                lv_obj_remove_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    // ============ ИНДИКАТОРЫ И ДИАГНОСТИКА (ВСЕГДА) ============
    CarData_Lock(10);
    float bat_voltage = data->batValue;
    float fuel_level = data->fuelValue;
    int temp_coolant = data->tempValue;
    int speed_kmh = data->speedValue;
    int current_rpm = data->rpmValue;
    uint32_t odo_val = data->odoKm;
    bool arduino_alive = data->uartArduinoAlive;
    bool trip_state = data->tripState;
    float current_accel_x = data->accel_x;
    float current_accel_y = data->accel_y;
    float current_accel_z = data->accel_z;
    uint16_t light_raw = (uint16_t)smooth_light;
    CarData_Unlock();

    // Обновляем массив индикаторов
    indicators_ok[0] = sd_card_mounted();
    indicators_ok[1] = arduino_alive;
    indicators_ok[2] = uart_is_gateway_alive();
    indicators_ok[3] = fiona_soul_phrases_check();
    indicators_ok[4] = sd_stats_check();
    indicators_ok[5] = sd_presets_check();

    // --- Новый формат виджета ESP32 ---
    snprintf(esp32_status_text, sizeof(esp32_status_text),
             "Speed: %d km/h\n"
             "RPM: %d\n"
             "Battery: %.1f V\n"
             "Fuel: %.1f L\n"
             "ODO: %u km\n"
             "Accel X:%.2f Y:%.2f Z:%.2f m/s²\n"
             "Gyro X:%.2f Y:%.2f Z:%.2f °/s\n"
             "Tilt R:%d P:%d deg\n"
             "Light: %d",
             speed_kmh, current_rpm,
             bat_voltage, fuel_level, odo_val,
             current_accel_x, current_accel_y, current_accel_z,
             data->gyro_x, data->gyro_y, data->gyro_z,
             data->tilt_roll, data->tilt_pitch,
             light_raw);

    snprintf(arduino_status_text, sizeof(arduino_status_text),
             "Статус: %s", arduino_alive ? "На связи" : "Нет связи");

    // --- Новый формат виджета Экран ---
    snprintf(screen_status_text, sizeof(screen_status_text),
             "Uptime: %lu sec\n"
             "SD: %s\n"
             "GW: %s\n"
             "Speech: %s\n"
             "Stat: %s\n"
             "Preset: %s\n"
             "Calib: 0x%02X",
             (uint32_t)(now - data->systemTime + 1),
             indicators_ok[0] ? "OK" : "ERR",
             indicators_ok[2] ? "OK" : "ERR",
             indicators_ok[3] ? "OK" : "ERR",
             indicators_ok[4] ? "OK" : "ERR",
             indicators_ok[5] ? "OK" : "ERR",
             data->calib_status);

    lv_textarea_set_text(ui_System_Textarea_TextAreaArduino, arduino_status_text);
    lv_textarea_set_text(ui_System_Textarea_TextAreaESP, esp32_status_text);
    lv_textarea_set_text(ui_System_Textarea_TextAreaScreen, screen_status_text);

    // Индикаторы (мигание) – тоже всегда
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