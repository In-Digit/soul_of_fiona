/**
 * @file ui_events.c
 * @brief Обработчики событий GUI с ленивой загрузкой экранов, живым дебагом,
 *        индикацией состояния устройств на DebugScreen, калибровкой IMU,
 *        ручным тестированием UART-пинов и полными реализациями климата/заправки/яркости.
 *
 * Для дебаг-экрана используется отдельный таймер `debug_data_timer`, который
 * обновляет системное время, запрашивает свежую телеметрию и обновляет текст/индикаторы.
 */

#include "ui.h"
#include "fiona_core.h"
#include "system_actions.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "protocol.h"
#include "sd_utils.h"
#include "fiona_brain.h"
#include "fiona_soul.h"
#include "fiona_phrase_loader.h"
#include "gps_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "ntp_sync.h"
#include "driver/uart.h"

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
extern void fiona_core_dashboard_on_load(void);
extern void fiona_core_refresh_dashboard(void);
extern void fiona_core_save_car_data_to_nvs(void);
extern void fiona_core_activate_screensaver(void);
extern void fiona_core_deactivate_screensaver(void);

// ---------- Переменные для ручного выбора стиля ----------
static uint8_t selected_style = 0;
static lv_timer_t * style_confirm_timer = NULL;

// ---------- Переменные для калибровки ----------
static int8_t calib_selected_index = -1;
static uint8_t calib_current_value = 0;
static bool calib_active = false;

// ---------- Внешние счётчики бездействия ----------
extern uint32_t inactivity_seconds;

// ---------- Время старта системы ----------
extern uint32_t boot_time;

// ---------- Дебаг-источник ----------
uint8_t debug_source = 0; // 0=Arduino, 1=ESP32, 2=Screen, 3=Speech, 4=Stats, 5=Presets

// ---------- Таймер для живого обновления дебаг-экрана ----------
static lv_timer_t * debug_data_timer = NULL;

// ---------- Таймер для калибровки IMU ----------
static lv_timer_t * calib_timer = NULL;

// ---------- Состояние UART-теста ----------
static bool uart_test_active = false;
static bool uart_test_success = false;
static int  uart_test_uart_num = -1;
static lv_timer_t * uart_test_timer = NULL;
static int  uart_test_device = 0;   // 0=Arduino, 1=Gateway, 2=GPS
static int  uart_test_rx = 33;
static int  uart_test_tx = 32;
static int  uart_test_baud = 115200;
static bool uart_test_dirty = false;

// Прототипы внутренних функций
static void style_confirm_timer_cb(lv_timer_t * t);
static int get_calib_device(void);
static int get_point_type(void);
static uint8_t get_calib_cmd(int point_type, int device);
static void apply_calib_save(void);
static void show_keyboard(lv_obj_t * textarea);
static void hide_keyboard(void);
void debug_update_textarea(void);
void debug_update_indicators(void);
static void debug_data_timer_cb(lv_timer_t * timer);
static void debug_data_timer_start(void);
void debug_data_timer_stop(void);
static void calib_timer_cb(lv_timer_t * timer);
static void calib_timer_start(void);
static void calib_timer_stop(void);

// UART-тест
static void uart_test_timer_cb(lv_timer_t * timer);
static void uart_test_start(void);
void uart_test_stop(void);

// ---------- Глобальная клавиатура ----------
static lv_obj_t * global_keyboard = NULL;

// ---------- Индикаторы для DebugScreen ----------
typedef struct {
    lv_obj_t *back;
    lv_obj_t *grey;
    lv_obj_t *red;
} debug_indicator_t;

#define DEBUG_INDICATOR_COUNT 6
static debug_indicator_t debug_indicators[DEBUG_INDICATOR_COUNT];
static bool debug_indicators_initialized = false;

static void debug_init_indicators(void) {
    debug_indicators[0].back = ui_DebugScreen_Image_SDBack;
    debug_indicators[0].grey = ui_DebugScreen_Image_SDGrey;
    debug_indicators[0].red  = ui_DebugScreen_Image_SDRed;

    debug_indicators[1].back = ui_DebugScreen_Image_ArduinoBack;
    debug_indicators[1].grey = ui_DebugScreen_Image_ArduinoGrey;
    debug_indicators[1].red  = ui_DebugScreen_Image_ArduinoRed;

    debug_indicators[2].back = ui_DebugScreen_Image_ESP32Back;
    debug_indicators[2].grey = ui_DebugScreen_Image_ESP32Grey;
    debug_indicators[2].red  = ui_DebugScreen_Image_ESP32Red;

    debug_indicators[3].back = ui_DebugScreen_Image_SpeachBack;
    debug_indicators[3].grey = ui_DebugScreen_Image_SpeachGrey;
    debug_indicators[3].red  = ui_DebugScreen_Image_SpeachRed;

    debug_indicators[4].back = ui_DebugScreen_Image_StatBack;
    debug_indicators[4].grey = ui_DebugScreen_Image_StatGrey;
    debug_indicators[4].red  = ui_DebugScreen_Image_StatRed;

    debug_indicators[5].back = ui_DebugScreen_Image_PresetBack;
    debug_indicators[5].grey = ui_DebugScreen_Image_PresetGrey;
    debug_indicators[5].red  = ui_DebugScreen_Image_PresetRed;

    debug_indicators_initialized = true;
}

/* ---------------------------------------------------------------------------
 * Вспомогательные функции калибровки
 * --------------------------------------------------------------------------- */
static int get_calib_device(void) {
    if (calib_selected_index < 0 || calib_selected_index > 7) return -1;
    if (calib_selected_index < 4) return 0;
    return 1;
}

static int get_point_type(void) {
    if (calib_selected_index < 0) return 0;
    return calib_selected_index % 4;
}

static uint8_t get_calib_cmd(int point_type, int device) {
    if (device == 0) {
        switch (point_type) {
            case 0: return MSG_FAN_CALIB_START_POINT;
            case 1: return MSG_FAN_CALIB_STOP_POINT;
            case 2: return MSG_FAN_CALIB_NOISE_LOW;
            case 3: return MSG_FAN_CALIB_NOISE_HIGH;
        }
    } else {
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
    lv_obj_add_flag(ui_Setting_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_Setting_Label_FanDefault, LV_OBJ_FLAG_HIDDEN);
    calib_active = false;
}

/* ---------------------------------------------------------------------------
 * Управление клавиатурой
 * --------------------------------------------------------------------------- */
static void show_keyboard(lv_obj_t * textarea) {
    if (global_keyboard != NULL) {
        lv_obj_del(global_keyboard);
        global_keyboard = NULL;
    }
    if (textarea == NULL) return;

    lv_obj_t * parent = lv_scr_act();
    global_keyboard = lv_keyboard_create(parent);
    lv_keyboard_set_textarea(global_keyboard, textarea);
    lv_keyboard_set_mode(global_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_width(global_keyboard, 780);
    lv_obj_set_height(global_keyboard, 226);
    lv_obj_set_align(global_keyboard, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_bg_color(global_keyboard, lv_color_hex(0x1C197A), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(global_keyboard, 255, LV_PART_ITEMS);
    lv_obj_set_style_text_color(global_keyboard, lv_color_hex(0x00D0FB), LV_PART_ITEMS);
    lv_obj_set_style_text_font(global_keyboard, &lv_font_montserrat_48, LV_PART_ITEMS);
}

static void hide_keyboard(void) {
    if (global_keyboard != NULL) {
        lv_obj_del(global_keyboard);
        global_keyboard = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * Сброс таймера бездействия
 * --------------------------------------------------------------------------- */
static void reset_inactivity_timer(void) {
    extern uint32_t inactivity_seconds;
    inactivity_seconds = 0;
}

/* ================================================================
 * ЖИВОЕ ОБНОВЛЕНИЕ ДЕБАГ-ЭКРАНА
 * ================================================================ */
void debug_update_textarea(void) {
    if (!ui_DebugScreen_Textarea_TextAreaScreen) return;

    CarData *data = CarData_Get();
    if (!data) return;

    char buf[1024] = {0};

    if (debug_source == 0) {
        CarData_Lock(10);
        snprintf(buf, sizeof(buf),
                 "=== ARDUINO LOG ===\nLink: %s\nFan1 PWM: %d (%d%%)\nFan2 PWM: %d (%d%%)\nCoolant Temp: %.1f C\nMode: %s (%d)\nAuto: %s\nTemp Offset: %.1f\nOffset Received: %s\n",
                 data->uartArduinoAlive ? "OK" : "NO",
                 data->fanCurrentPWM1, (data->fanCurrentPWM1*100)/255,
                 data->fanCurrentPWM2, (data->fanCurrentPWM2*100)/255,
                 data->arduino_coolant_temp,
                 data->arduino_fan_mode==1?"NORMAL":(data->arduino_fan_mode==2?"HIGHWAY":"CITY"),
                 data->arduino_fan_mode,
                 data->arduino_mode_from_screen?"SCREEN":"AUTO",
                 data->arduino_temp_offset,
                 data->arduino_offset_received?"Yes":"No");
        CarData_Unlock();
    } else if (debug_source == 1) {
        CarData_Lock(10);
        snprintf(buf, sizeof(buf), "=== ESP32 LOG ===\nSpeed: %d km/h\nRPM: %d\nCoolant: %d C\nBattery: %.1f V\nFuel: %.1f L\nODO: %u km\nRange: %d km\nLPH: %.1f l/h\nThrottle: %.1f%%\nCabin Temp: %.1f C\nTarget Temp: %.1f C\nHeater PWM: %d\nOBD: %s\nInternet: %s\n",
                 data->speedValue, data->rpmValue, data->tempValue, data->batValue,
                 data->fuelValue, data->odoKm, data->rangeValue, data->lphValue,
                 data->throttlePos, data->cabin_temp, data->climate_target_temp,
                 data->heater_pwm,
                 data->obdConnected?"Yes":"No", data->internetAvailable?"Yes":"No");
        CarData_Unlock();
    } else if (debug_source == 2) {
        CarData_Lock(10);
        uint32_t now = data->systemTime;
        uint32_t uptime = (now > boot_time) ? (now - boot_time) : 0;
        int buf_len = snprintf(buf, sizeof(buf),
                 "=== SCREEN LOG ===\n"
                 "Uptime: %lu s\n"
                 "WiFi: %s\n"
                 "GW Alive: %s\n"
                 "Arduino Alive: %s\n"
                 "Screensaver: %s\n"
                 "Calib: 0x%02X\n"
                 "Driving Mode: %d\n"
                 "Driving Style: %d\n"
                 "Time Synced: %s\n"
                 "Light Raw: %d\n"
                 "GPS Valid: %s\n"
                 "GPS Alive: %s\n"
                 "GPS Log:\n",
                 uptime,
                 data->wifiConnected?"Yes":"No",
                 uart_is_gateway_alive()?"Yes":"No",
                 data->uartArduinoAlive?"Yes":"No",
                 screensaver_active?"Yes":"No",
                 data->calib_status,
                 fiona_brain_get_state()?fiona_brain_get_state()->driving_mode:-1,
                 fiona_brain_get_state()?fiona_brain_get_state()->driving_style:-1,
                 data->time_synced?"Yes":"No",
                 (int)data->ambient_light_raw,
                 data->gps_valid?"Yes":"No",
                 gps_is_alive()?"Yes":"No");
        // Кольцевой буфер NMEA-строк
        int count = data->gps_nmea_buf_count;
        int start = (data->gps_nmea_buf_idx - count + GPS_NMEA_BUF_SIZE) % GPS_NMEA_BUF_SIZE;
        for (int i = 0; i < count && buf_len < sizeof(buf)-40; i++) {
            int idx = (start + i) % GPS_NMEA_BUF_SIZE;
            const char *line = data->gps_nmea_buf[idx];
            buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "  %s\n", line);
        }
        if (count == 0) {
            buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "  (no data)\n");
        }
        // Координаты, если есть валидность
        snprintf(buf + buf_len, sizeof(buf) - buf_len,
                 "Lat: %.6f\n"
                 "Lon: %.6f\n"
                 "Speed: %.1f km/h\n"
                 "Sats: %d\n",
                 data->gps_valid ? data->gps_lat : 0.0,
                 data->gps_valid ? data->gps_lon : 0.0,
                 data->gps_valid ? data->gps_speed : 0.0f,
                 data->gps_valid ? data->gps_satellites : 0);
        CarData_Unlock();
    } else if (debug_source == 3) {
        snprintf(buf, sizeof(buf), "=== SPEECH LOG ===\nPhrases loaded: %d\n", phrase_loader_event_count());
    } else if (debug_source == 4) {
        CarData_Lock(10);
        snprintf(buf, sizeof(buf), "=== STATS ===\nTrip: %s\nTrip Dist: %.1f km\nTrip Fuel: %.2f L\nDrive Cycles: %d\n",
                 data->tripState?"Active":"Inactive", data->tripDistanceKm, data->tripFuelUsed, data->receivedDriveCycleCount);
        CarData_Unlock();
    } else if (debug_source == 5) {
        snprintf(buf, sizeof(buf), "=== PRESETS ===\nActive Preset ID: %d\n", data->activePresetId);
    }
    lv_textarea_set_text(ui_DebugScreen_Textarea_TextAreaScreen, buf);
    lv_obj_invalidate(ui_DebugScreen_Textarea_TextAreaScreen);
}

void debug_update_indicators(void) {
    debug_init_indicators();

    bool indicators_ok[6];
    CarData *data = CarData_Get();
    if (data) {
        CarData_Lock(10);
        indicators_ok[0] = sd_card_mounted();
        indicators_ok[1] = data->uartArduinoAlive;
        indicators_ok[2] = uart_is_gateway_alive();
        indicators_ok[3] = fiona_soul_phrases_check();
        indicators_ok[4] = sd_stats_check();
        indicators_ok[5] = sd_presets_check();
        CarData_Unlock();
    } else {
        for (int i = 0; i < 6; i++) indicators_ok[i] = false;
    }

    static bool blink_toggle = false;
    blink_toggle = !blink_toggle;

    for (int i = 0; i < DEBUG_INDICATOR_COUNT; i++) {
        lv_obj_t *back = debug_indicators[i].back;
        lv_obj_t *grey = debug_indicators[i].grey;
        lv_obj_t *red  = debug_indicators[i].red;
        if (!back || !grey || !red) continue;

        lv_obj_add_flag(back, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(grey, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(red, LV_OBJ_FLAG_HIDDEN);

        if (i == debug_source) {
            lv_obj_remove_flag(back, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        if (indicators_ok[i]) {
            lv_obj_remove_flag(back, LV_OBJ_FLAG_HIDDEN);
        } else {
            if (blink_toggle) {
                lv_obj_remove_flag(red, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_remove_flag(grey, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

/* ----------------------------------------------------------------- */
/* Таймер для живого обновления дебаг-экрана                          */
/* ----------------------------------------------------------------- */
static void debug_data_timer_cb(lv_timer_t * timer) {
    // Обновляем системное время
    CarData *data = CarData_Get();
    if (data) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        CarData_Lock(10);
        data->systemTime = (uint32_t)tv.tv_sec;
        CarData_Unlock();
    }

    // Запрашиваем свежую телеметрию (как на дашборде, но без виджетов)
    if (uart_is_gateway_alive()) {
        uart_send_to_gateway(MSG_SPEED, NULL, 0);
        uart_send_to_gateway(MSG_RPM, NULL, 0);
        uart_send_to_gateway(MSG_COOLANT_TEMP, NULL, 0);
        uart_send_to_gateway(MSG_VOLTAGE, NULL, 0);
        uart_send_to_gateway(MSG_FUEL_LEVEL, NULL, 0);
        uart_send_to_gateway(MSG_ODO, NULL, 0);
        uart_send_to_gateway(MSG_RANGE, NULL, 0);
        uart_send_to_gateway(MSG_INST_FUEL, NULL, 0);
        uart_send_to_gateway(MSG_THROTTLE, NULL, 0);
        uart_send_to_gateway(MSG_CLIMATE_TELEMETRY, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_TIME, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_PAUSE, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_FUEL, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_DIST, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_STATUS, NULL, 0);
        uart_send_to_gateway(MSG_TRIP_COST, NULL, 0);
    }
    if (uart_is_arduino_alive()) {
        uart_send_to_arduino(MSG_FAN_TELEMETRY, NULL, 0);
    }

    debug_update_textarea();
    debug_update_indicators();
}

static void debug_data_timer_start(void) {
    if (debug_data_timer != NULL) {
        lv_timer_del(debug_data_timer);
        debug_data_timer = NULL;
    }
    debug_data_timer = lv_timer_create(debug_data_timer_cb, 1000, NULL);
}

void debug_data_timer_stop(void) {
    if (debug_data_timer != NULL) {
        lv_timer_del(debug_data_timer);
        debug_data_timer = NULL;
    }
}

/* ----------------------------------------------------------------- */
/* Таймер для калибровки IMU                                         */
/* ----------------------------------------------------------------- */
static void calib_timer_cb(lv_timer_t * timer) {
    uart_send_to_gateway(MSG_REQ_CALIB_STATUS, NULL, 0);

    CarData *data = CarData_Get();
    if (!data || !ui_Setting_Label_KalibLabel) return;

    const char *msg = NULL;
    bool finished = false;

    switch (data->calib_status) {
        case 0x00: msg = "Ожидание запуска..."; break;
        case 0x01: msg = "Фаза 1: не двигайтесь"; break;
        case 0x02: msg = "Фаза 2: разгон и торможение"; break;
        case 0x03: msg = "Калибровка успешно завершена!"; finished = true; break;
        case 0x10: msg = "Ошибка: нет датчика"; finished = true; break;
        case 0x11: msg = "Ошибка: провал фазы 1"; finished = true; break;
        case 0x12: msg = "Ошибка: провал фазы 2"; finished = true; break;
        case 0x13: msg = "Ошибка калибровки"; finished = true; break;
        default:   msg = "Неизвестный статус"; break;
    }

    lv_label_set_text(ui_Setting_Label_KalibLabel, msg);

    if (finished) {
        calib_timer_stop();
    }
}

static void calib_timer_start(void) {
    calib_timer_stop();
    calib_timer = lv_timer_create(calib_timer_cb, 500, NULL);
    uart_send_to_gateway(MSG_REQ_CALIB_STATUS, NULL, 0);
}

static void calib_timer_stop(void) {
    if (calib_timer != NULL) {
        lv_timer_del(calib_timer);
        calib_timer = NULL;
    }
}

/* ================================================================
 * UART ТЕСТИРОВАНИЕ
 * ================================================================ */
static void uart_test_timer_cb(lv_timer_t * timer) {
    uint8_t buf[256];
    int len = uart_read_bytes(uart_test_uart_num, buf, sizeof(buf), 0);
    if (len <= 0) return;

    bool got_valid = false;
    if (uart_test_device == 2) {
        for (int i = 0; i < len; i++) {
            if (buf[i] == '$') {
                char line[128] = {0};
                int j = 0;
                for (; i < len && j < 127; i++, j++) {
                    line[j] = buf[i];
                    if (buf[i] == '\n') break;
                }
                if (strncmp(line, "$GPRMC", 6) == 0 || strncmp(line, "$GPGGA", 6) == 0) {
                    got_valid = true;
                    if (ui_Setting_Textarea_TextAreaUartLog) {
                        lv_textarea_add_text(ui_Setting_Textarea_TextAreaUartLog, line);
                        lv_textarea_add_text(ui_Setting_Textarea_TextAreaUartLog, "\n");
                    }
                }
            }
        }
    } else {
        for (int i = 0; i <= len - FRAME_TOTAL_SIZE; i++) {
            if (buf[i] == FRAME_MAGIC) {
                uint8_t crc = crc8_calculate(&buf[i], FRAME_TOTAL_SIZE - 1);
                if (crc == buf[i + FRAME_TOTAL_SIZE - 1]) {
                    got_valid = true;
                    char log[64];
                    snprintf(log, sizeof(log), "RX: 0x%02X\n", buf[i+1]);
                    if (ui_Setting_Textarea_TextAreaUartLog)
                        lv_textarea_add_text(ui_Setting_Textarea_TextAreaUartLog, log);
                }
            }
        }
    }

    if (got_valid && !uart_test_success) {
        uart_test_success = true;
        if (ui_Setting_Textarea_TextAreaUartLog) {
            lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x00FF00), 0);
            lv_textarea_add_text(ui_Setting_Textarea_TextAreaUartLog, "Связь установлена!\n");
        }
    }
}

static void uart_test_start(void) {
    uart_test_stop();
    uart_test_dirty = false;
    uart_config_t uart_config = {
        .baud_rate = uart_test_baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int uart_num = (uart_test_device == 0) ? UART_NUM_2 : (uart_test_device == 1 ? UART_NUM_1 : UART_NUM_0);
    uart_driver_install(uart_num, 256, 0, 0, NULL, 0);
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, uart_test_tx, uart_test_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_test_uart_num = uart_num;
    uart_test_active = true;
    uart_test_success = false;
    uart_test_timer = lv_timer_create(uart_test_timer_cb, 200, NULL);

    if (ui_Setting_Textarea_TextAreaUartLog) {
        lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, "");
        lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0xFFFF00), 0);
        lv_textarea_add_text(ui_Setting_Textarea_TextAreaUartLog, "Тест запущен...\n");
    }
}

void uart_test_stop(void) {
    if (uart_test_active) {
        if (uart_test_uart_num >= 0) {
            uart_driver_delete(uart_test_uart_num);
            uart_test_uart_num = -1;
        }
        if (uart_test_timer) {
            lv_timer_del(uart_test_timer);
            uart_test_timer = NULL;
        }
        uart_test_active = false;
    }
}

/* ================================================================
 * ЛЕНИВАЯ ЗАГРУЗКА ЭКРАНОВ
 * ================================================================ */
static void switch_to_secondary_screen(lv_obj_t ** target_screen_ptr,
                                       void (*target_init)(void),
                                       void (*target_destroy)(void),
                                       lv_screen_load_anim_t anim,
                                       int spd, int delay)
{
    // Останавливаем таймер дебаг-экрана, калибровки и UART-тест
    debug_data_timer_stop();
    calib_timer_stop();
    uart_test_stop();

    if (poll_timer) { lv_timer_del(poll_timer); poll_timer = NULL; }
    if (clock_timer) { lv_timer_del(clock_timer); clock_timer = NULL; }

    fiona_observers_unsubscribe_all();

    extern lv_obj_t * ui_Screen_SplashScreen;
    if (ui_Screen_SplashScreen) {
        lv_disp_load_scr(ui_Screen_SplashScreen);
    }

    extern lv_obj_t * ui_Screen_ScreenReFuel;
    extern lv_obj_t * ui_Screen_Setting;
    extern lv_obj_t * ui_Screen_DebugScreen;
    extern lv_obj_t * ui_Screen_DashBoard;

    if (ui_Screen_ScreenReFuel) { ui_Screen_ScreenReFuel_screen_destroy(); ui_Screen_ScreenReFuel = NULL; }
    if (ui_Screen_Setting)    { ui_Screen_Setting_screen_destroy();    ui_Screen_Setting = NULL; }
    if (ui_Screen_DebugScreen){ ui_Screen_DebugScreen_screen_destroy();ui_Screen_DebugScreen = NULL; }
    if (ui_Screen_DashBoard)  { ui_Screen_DashBoard_screen_destroy();  ui_Screen_DashBoard = NULL; }

    target_init();

    if (*target_screen_ptr == ui_Screen_DashBoard) {
        fiona_observers_init_widgets();
        fiona_observers_subscribe_all();
        fiona_background_init_timers();
    }

    lv_screen_load_anim(*target_screen_ptr, anim, spd, delay, false);
}

static void return_to_dashboard(lv_screen_load_anim_t anim, int spd, int delay)
{
    switch_to_secondary_screen(&ui_Screen_DashBoard,
                               ui_Screen_DashBoard_screen_init,
                               ui_Screen_DashBoard_screen_destroy,
                               anim, spd, delay);
}

/* ================================================================
 * Обработчики событий
 * ================================================================ */

// ------------------- Дашборд -------------------
void DashOnFocused(lv_event_t * e) { reset_inactivity_timer(); }
void OnDefocused(lv_event_t * e) {}
void DashboardOnLoad(lv_event_t * e) { fiona_core_dashboard_on_load(); }

void GetInternetTime(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(10);
    bool currently_forced = data->trip_force_active;
    data->trip_force_active = !currently_forced;
    bool new_state = data->trip_force_active;
    CarData_Unlock();

    if (new_state) {
        uint8_t payload = 0x01;
        uart_send_to_gateway(MSG_TRIP_TOGGLE, &payload, 1);
    } else {
        uint8_t payload = 0x02;
        uart_send_to_gateway(MSG_TRIP_TOGGLE, &payload, 1);
    }

    fiona_core_save_car_data_to_nvs();
}

void OnTripClick(lv_event_t * e) {
    reset_inactivity_timer();
    uart_send_to_gateway(MSG_TRIP_TOGGLE, NULL, 0);
}

// ------------------- Переходы на экраны -------------------
void ToRefuelClk(lv_event_t * e) {
    reset_inactivity_timer();
    switch_to_secondary_screen(&ui_Screen_ScreenReFuel,
                               ui_Screen_ScreenReFuel_screen_init,
                               ui_Screen_ScreenReFuel_screen_destroy,
                               LV_SCR_LOAD_ANIM_FADE_ON, 500, 0);
}

void ToSettingClk(lv_event_t * e) {
    reset_inactivity_timer();
    switch_to_secondary_screen(&ui_Screen_Setting,
                               ui_Screen_Setting_screen_init,
                               ui_Screen_Setting_screen_destroy,
                               LV_SCR_LOAD_ANIM_FADE_ON, 500, 0);
    // Инициализируем дропдауны UART
    if (ui_Setting_Dropdown_UartDevice) {
        lv_dropdown_set_selected(ui_Setting_Dropdown_UartDevice, uart_test_device);
    }
    if (ui_Setting_Dropdown_RXList) {
        const char *rx_options = "1\n2\n3\n4\n5\n26\n27\n32\n33\n37\n38\n45\n46\n47";
        char rx_str[8];
        snprintf(rx_str, sizeof(rx_str), "%d", uart_test_rx);
        int idx = 0;
        const char *opt = rx_options;
        while (*opt) {
            const char *next = strchr(opt, '\n');
            if (!next) next = opt + strlen(opt);
            if (strncmp(opt, rx_str, next - opt) == 0) {
                lv_dropdown_set_selected(ui_Setting_Dropdown_RXList, idx);
                break;
            }
            idx++;
            opt = (*next == '\n') ? next + 1 : next;
        }
    }
    if (ui_Setting_Dropdown_TXList) {
        const char *tx_options = "1\n2\n3\n4\n5\n26\n27\n32\n33\n37\n38\n45\n46\n47";
        char tx_str[8];
        snprintf(tx_str, sizeof(tx_str), "%d", uart_test_tx);
        int idx = 0;
        const char *opt = tx_options;
        while (*opt) {
            const char *next = strchr(opt, '\n');
            if (!next) next = opt + strlen(opt);
            if (strncmp(opt, tx_str, next - opt) == 0) {
                lv_dropdown_set_selected(ui_Setting_Dropdown_TXList, idx);
                break;
            }
            idx++;
            opt = (*next == '\n') ? next + 1 : next;
        }
    }
    if (ui_Setting_Dropdown_SpeedUart) {
        const char *spd_options = "9600\n115200\n921600";
        char spd_str[8];
        snprintf(spd_str, sizeof(spd_str), "%d", uart_test_baud);
        int idx = 0;
        const char *opt = spd_options;
        while (*opt) {
            const char *next = strchr(opt, '\n');
            if (!next) next = opt + strlen(opt);
            if (strncmp(opt, spd_str, next - opt) == 0) {
                lv_dropdown_set_selected(ui_Setting_Dropdown_SpeedUart, idx);
                break;
            }
            idx++;
            opt = (*next == '\n') ? next + 1 : next;
        }
    }

    if (ui_Setting_Textarea_TextAreaUartLog) {
        lv_obj_add_flag(ui_Setting_Textarea_TextAreaUartLog, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x808080), 0);
        lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, "Кликните для теста");
    }
}

void ToDebugClk(lv_event_t * e) {
    reset_inactivity_timer();
    switch_to_secondary_screen(&ui_Screen_DebugScreen,
                               ui_Screen_DebugScreen_screen_init,
                               ui_Screen_DebugScreen_screen_destroy,
                               LV_SCR_LOAD_ANIM_FADE_ON, 500, 0);
    debug_init_indicators();
    debug_update_indicators();
    debug_update_textarea();
    // Запускаем отдельный таймер для живого обновления дебаг-экрана
    debug_data_timer_start();
}

void ToHomeClick(lv_event_t * e) {
    reset_inactivity_timer();
    lv_obj_t * act_scr = lv_scr_act();
    if (act_scr == ui_Screen_DashBoard) {
        lv_obj_add_flag(ui_DashBoard_Container_ContainerClimate, LV_OBJ_FLAG_HIDDEN);
    } else {
        return_to_dashboard(LV_SCR_LOAD_ANIM_FADE_ON, 500, 0);
    }
}

// ------------------- Заправка -------------------
void OnTankFocus(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", data->fuelValue);
    lv_textarea_set_text(ui_ScreenReFuel_Textarea_TACurrentFuel, buf);
    uint32_t miles = (uint32_t)(data->odoKm * 0.621371f);
    snprintf(buf, sizeof(buf), "%u", miles);
    lv_textarea_set_text(ui_ScreenReFuel_Textarea_TAODO, buf);
    snprintf(buf, sizeof(buf), "%.2f", data->fuelPrice);
    lv_textarea_set_text(ui_ScreenReFuel_Textarea_TAPrise, buf);
    lv_textarea_set_text(ui_ScreenReFuel_Textarea_TAFuelToAdd, "");
    lv_obj_add_flag(ui_ScreenReFuel_Label_RefSaveLBtn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_state(ui_ScreenReFuel_Switch_SwitchFullTank, LV_STATE_CHECKED);
    show_keyboard(ui_ScreenReFuel_Textarea_TACurrentFuel);
}
void TankDeFocus(lv_event_t * e) {}
void OnPriseFocus(lv_event_t * e) { reset_inactivity_timer(); show_keyboard(ui_ScreenReFuel_Textarea_TAPrise); }
void OnPriseDeFocus(lv_event_t * e) { hide_keyboard(); }
void OnAddedFocus(lv_event_t * e) { reset_inactivity_timer(); show_keyboard(ui_ScreenReFuel_Textarea_TAFuelToAdd); }
void OnAddedDeFocus(lv_event_t * e) { hide_keyboard(); }
void OnOdoFocus(lv_event_t * e) { reset_inactivity_timer(); show_keyboard(ui_ScreenReFuel_Textarea_TAODO); }
void OnOdoDeFocus(lv_event_t * e) { hide_keyboard(); }
void OnCalibrateClicked(lv_event_t * e) { reset_inactivity_timer(); }

void OnSaveClk(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;
    const char *fuel_add_text = lv_textarea_get_text(ui_ScreenReFuel_Textarea_TAFuelToAdd);
    const char *fuel_curr_text = lv_textarea_get_text(ui_ScreenReFuel_Textarea_TACurrentFuel);
    const char *odo_text = lv_textarea_get_text(ui_ScreenReFuel_Textarea_TAODO);
    const char *price_text = lv_textarea_get_text(ui_ScreenReFuel_Textarea_TAPrise);
    bool switch_full_tank = lv_obj_has_state(ui_ScreenReFuel_Switch_SwitchFullTank, LV_STATE_CHECKED);

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
        return_to_dashboard(LV_SCR_LOAD_ANIM_FADE_ON, 500, 0);
    }
}

// ------------------- Настройки времени -------------------
void TimeOnFocus(lv_event_t * e) { reset_inactivity_timer(); show_keyboard(ui_Setting_Textarea_TimeTextArea); }
void TimeOnDeFocus(lv_event_t * e) { hide_keyboard(); }
void OnTimeApply(lv_event_t * e) { reset_inactivity_timer(); system_actions_set_time_from_textarea(); }
void OnTimeAsk(lv_event_t * e) { reset_inactivity_timer(); ntp_sync_request(); }

// ------------------- Настройки яркости -------------------
void OnSaveClick(lv_event_t * e) {
    reset_inactivity_timer();
    fiona_core_save_car_data_to_nvs();
    lv_label_set_text(ui_Setting_Label_LightLbl2, "Сохранено!");
}
void OnLightSliderThresholdChanged(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;
    int val = lv_slider_get_value(ui_Setting_Slider_SliderLight);
    if (val < 0) val = 0;
    if (val > 4095) val = 4095;
    CarData_Lock(10);
    data->light_threshold_bright = (uint8_t)val;
    CarData_Unlock();
    lv_label_set_text_fmt(ui_Setting_Label_LightMarkLbl2, "Светло: %d", val);
}
void OnSliderThresholdChanged(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;
    int val = lv_slider_get_value(ui_Setting_Slider_SliderDark);
    if (val < 0) val = 0;
    if (val > 4095) val = 4095;
    CarData_Lock(10);
    data->light_threshold_dark = (uint8_t)val;
    CarData_Unlock();
    lv_label_set_text_fmt(ui_Setting_Label_DarkMarkLbl2, "Темно: %d", val);
}
void OnLightSliderChanged(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;
    int val = lv_slider_get_value(ui_Setting_Slider_SliderBridges);
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    CarData_Lock(10);
    data->backlight_brightness = (uint8_t)val;
    CarData_Unlock();
    lv_label_set_text_fmt(ui_Setting_Label_LightLbl2, "Яркость: %d", val);
}

/* ---------------------------------------------------------------------------
 * Климат-контроль (слой дашборда)
 * --------------------------------------------------------------------------- */
void TempChanged(lv_event_t * e) {
    reset_inactivity_timer();
    int val = lv_slider_get_value(ui_DashBoard_Slider_TempSlider);
    float target = 22.0f + (float)val;
    if (target < 10.0f) target = 10.0f;
    if (target > 35.0f) target = 35.0f;
    int16_t temp_x10 = (int16_t)(target * 10.0f);
    uint8_t payload[2] = { (uint8_t)(temp_x10 & 0xFF), (uint8_t)((temp_x10 >> 8) & 0xFF) };
    uart_send_to_gateway(MSG_CLIMATE_SET_TEMP, payload, 2);
    CarData *data = CarData_Get();
    if (data) { CarData_Lock(10); data->climate_target_temp = target; CarData_Unlock(); }
    if (lv_obj_has_state(ui_DashBoard_Switch_TempSwitch, LV_STATE_CHECKED)) {
        lv_obj_remove_state(ui_DashBoard_Switch_TempSwitch, LV_STATE_CHECKED);
        uint8_t auto_off = 0; uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_off, 1);
    }
    if (ui_DashBoard_Label_Temperatura) lv_label_set_text_fmt(ui_DashBoard_Label_Temperatura, "%.1f C", target);
}
void AlignChanged(lv_event_t * e) { reset_inactivity_timer(); }
void FlowChanged(lv_event_t * e) {
    reset_inactivity_timer();
    uint8_t pwm = (uint8_t)lv_slider_get_value(ui_DashBoard_Slider_FlowSlider);
    uart_send_to_gateway(MSG_CLIMATE_SET_PWM, &pwm, 1);
    CarData *data = CarData_Get();
    if (data) { CarData_Lock(10); data->heater_pwm = pwm; CarData_Unlock(); }
    if (lv_obj_has_state(ui_DashBoard_Switch_FlpwSwitch, LV_STATE_CHECKED)) {
        lv_obj_remove_state(ui_DashBoard_Switch_FlpwSwitch, LV_STATE_CHECKED);
        uint8_t auto_off = 0; uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_off, 1);
    }
    if (ui_DashBoard_Label_FlowShim) lv_label_set_text_fmt(ui_DashBoard_Label_FlowShim, "%d%%", (pwm * 100) / 255);
}
void TempAutoChange(lv_event_t * e) {
    reset_inactivity_timer();
    uint8_t auto_state = lv_obj_has_state(ui_DashBoard_Switch_TempSwitch, LV_STATE_CHECKED) ? 1 : 0;
    uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_state, 1);
}
void AlignAutoChange(lv_event_t * e) { reset_inactivity_timer(); }
void FlowAutoChange(lv_event_t * e) {
    reset_inactivity_timer();
    uint8_t auto_state = lv_obj_has_state(ui_DashBoard_Switch_FlpwSwitch, LV_STATE_CHECKED) ? 1 : 0;
    uart_send_to_gateway(MSG_CLIMATE_SET_AUTO, &auto_state, 1);
}

/* ---------------------------------------------------------------------------
 * Калибровка вентиляторов/печки (полные функции)
 * --------------------------------------------------------------------------- */
void FanChangeDot(lv_event_t * e) {
    reset_inactivity_timer();
    calib_selected_index = lv_dropdown_get_selected(ui_Setting_Dropdown_DropdownFanDot);
    CarData *data = CarData_Get();
    if (!data) return;
    int device = get_calib_device();
    int pt = get_point_type();
    if (device == 1) {
        if (!calib_active) {
            switch (pt) {
                case 0: calib_current_value = data->climateCalibStartPoint; break;
                case 1: calib_current_value = data->climateCalibStopPoint; break;
                case 2: calib_current_value = data->climateCalibNoiseLow; break;
                case 3: calib_current_value = data->climateCalibNoiseHigh; break;
            }
        }
    } else {
        calib_current_value = 0;
    }
    lv_label_set_text_fmt(ui_Setting_Label_FanCalibLbl, "%d", calib_current_value);
    lv_obj_add_flag(ui_Setting_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_Setting_Label_FanDefault, LV_OBJ_FLAG_HIDDEN);
}

void FanPlusClk(lv_event_t * e) {
    reset_inactivity_timer();
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    if (device == 1 && !calib_active) {
        uart_send_to_gateway(MSG_HEATER_CALIB_START, NULL, 0);
        calib_active = true;
    }
    if (calib_current_value < 255) calib_current_value++;
    lv_label_set_text_fmt(ui_Setting_Label_FanCalibLbl, "%d", calib_current_value);
    if (device == 1) {
        uint8_t step = 1;
        uart_send_to_gateway(MSG_HEATER_CALIB_STEP, &step, 1);
    }
    lv_obj_remove_flag(ui_Setting_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_Setting_Label_FanDefault, LV_OBJ_FLAG_HIDDEN);
}

void FanMinusClk(lv_event_t * e) {
    reset_inactivity_timer();
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    if (device == 1 && !calib_active) {
        uart_send_to_gateway(MSG_HEATER_CALIB_START, NULL, 0);
        calib_active = true;
    }
    if (calib_current_value > 0) calib_current_value--;
    lv_label_set_text_fmt(ui_Setting_Label_FanCalibLbl, "%d", calib_current_value);
    if (device == 1) {
        uint8_t step = (uint8_t)-1;
        uart_send_to_gateway(MSG_HEATER_CALIB_STEP, &step, 1);
    }
    lv_obj_remove_flag(ui_Setting_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_Setting_Label_FanDefault, LV_OBJ_FLAG_HIDDEN);
}

void FanSaveApply(lv_event_t * e) {
    reset_inactivity_timer();
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    int pt = get_point_type();
    uint8_t cmd = get_calib_cmd(pt, device);
    if (device == 0) {
        uart_send_to_arduino(cmd, &calib_current_value, 1);
        uart_send_to_arduino(MSG_FAN_CALIB_SAVE, NULL, 0);
    } else {
        uart_send_to_gateway(cmd, &calib_current_value, 1);
    }
    lv_obj_add_flag(ui_Setting_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_Setting_Label_FanDefault, LV_OBJ_FLAG_HIDDEN);
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

void FanDefaultClick(lv_event_t * e) {
    reset_inactivity_timer();
    if (calib_selected_index < 0) return;
    int device = get_calib_device();
    if (device == 0) {
        uint8_t start_pt = 65, stop_pt = 30, noise_low = 170, noise_high = 200;
        uart_send_to_arduino(MSG_FAN_CALIB_START_POINT, &start_pt, 1);
        uart_send_to_arduino(MSG_FAN_CALIB_STOP_POINT, &stop_pt, 1);
        uart_send_to_arduino(MSG_FAN_CALIB_NOISE_LOW, &noise_low, 1);
        uart_send_to_arduino(MSG_FAN_CALIB_NOISE_HIGH, &noise_high, 1);
        uart_send_to_arduino(MSG_FAN_CALIB_SAVE, NULL, 0);
    } else if (device == 1) {
        uint8_t start_pt = 230, stop_pt = 185, noise_low = 0, noise_high = 0;
        uart_send_to_gateway(MSG_HEATER_CALIB_START_POINT, &start_pt, 1);
        uart_send_to_gateway(MSG_HEATER_CALIB_STOP_POINT, &stop_pt, 1);
        uart_send_to_gateway(MSG_HEATER_CALIB_NOISE_LOW, &noise_low, 1);
        uart_send_to_gateway(MSG_HEATER_CALIB_NOISE_HIGH, &noise_high, 1);
        uart_send_to_gateway(MSG_HEATER_CALIB_SAVE, NULL, 0);
    }
    lv_obj_add_flag(ui_Setting_Label_FanApply, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_Setting_Label_FanDefault, LV_OBJ_FLAG_HIDDEN);
    calib_active = false;
}

/* ---------------------------------------------------------------------------
 * Дебаг-экран и кнопки (переключение источника с индикацией)
 * --------------------------------------------------------------------------- */
void OnDebugScreen(lv_event_t * e) {
    reset_inactivity_timer();
}

static void select_debug_source(uint8_t source) {
    debug_source = source;
    debug_update_indicators();
    debug_update_textarea();
}

void OnSdClick(lv_event_t * e)      { reset_inactivity_timer(); select_debug_source(2); }
void OnArduinoClk(lv_event_t * e)   { reset_inactivity_timer(); select_debug_source(0); }
void OnESPClk(lv_event_t * e)       { reset_inactivity_timer(); select_debug_source(1); }
void OnSpeachClk(lv_event_t * e)    { reset_inactivity_timer(); select_debug_source(3); }
void OnStatClk(lv_event_t * e)      { reset_inactivity_timer(); select_debug_source(4); }
void OnPresetsClk(lv_event_t * e)   { reset_inactivity_timer(); select_debug_source(5); }

/* ---------------------------------------------------------------------------
 * Скринсейвер
 * --------------------------------------------------------------------------- */
void OnScreenSaverClk(lv_event_t * e) { reset_inactivity_timer(); fiona_core_deactivate_screensaver(); }

/* ---------------------------------------------------------------------------
 * Калибровка IMU (новый механизм с выводом в KalibLabel)
 * --------------------------------------------------------------------------- */
void CalibrateMCUBtnClick(lv_event_t * e) {
    reset_inactivity_timer();
    uart_send_to_gateway(MSG_CALIBRATE_ACCEL, NULL, 0);
    extern bool calibration_active;
    calibration_active = true;
    calib_timer_start();
}

/* ---------------------------------------------------------------------------
 * Ручной выбор стиля (котики)
 * --------------------------------------------------------------------------- */
void RezChange(lv_event_t * e) {
    reset_inactivity_timer();
    selected_style = (selected_style + 1) % 4;
    lv_obj_add_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DashBoard_Image_ImageRed, LV_OBJ_FLAG_HIDDEN);
    if (selected_style == 1) lv_obj_remove_flag(ui_DashBoard_Image_ImageGreen, LV_OBJ_FLAG_HIDDEN);
    else if (selected_style == 2) lv_obj_remove_flag(ui_DashBoard_Image_ImageYellow, LV_OBJ_FLAG_HIDDEN);
    else if (selected_style == 3) lv_obj_remove_flag(ui_DashBoard_Image_ImageCyan, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
    if (style_confirm_timer) lv_timer_del(style_confirm_timer);
    style_confirm_timer = lv_timer_create(style_confirm_timer_cb, 5000, NULL);
}
static void style_confirm_timer_cb(lv_timer_t * t) {
    FionaState *state = fiona_brain_get_state();
    if (state) state->manual_style = selected_style;
    lv_obj_add_flag(ui_DashBoard_Image_ImageSpeed, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t); style_confirm_timer = NULL;
}

/* ---------------------------------------------------------------------------
 * Фокус на экране настроек
 * --------------------------------------------------------------------------- */
void SettingOnFocused(lv_event_t * e) {
    reset_inactivity_timer();
    CarData *data = CarData_Get();
    if (!data) return;
    CarData_Lock(10);
    uint8_t brightness = data->backlight_brightness;
    uint8_t threshold_bright = data->light_threshold_bright;
    uint8_t threshold_dark = data->light_threshold_dark;
    CarData_Unlock();
    lv_slider_set_value(ui_Setting_Slider_SliderBridges, brightness, LV_ANIM_OFF);
    lv_slider_set_value(ui_Setting_Slider_SliderLight, threshold_bright, LV_ANIM_OFF);
    lv_slider_set_value(ui_Setting_Slider_SliderDark, threshold_dark, LV_ANIM_OFF);
}
void SettingDeFocused(lv_event_t * e) {}
void ImageAnimOnLoad(lv_event_t * e) {}
void ImageAnimOnUnLoad(lv_event_t * e) {}

// ------------------- UART тестирование (исправленная логика) -------------------
void DeviceChange(lv_event_t * e) {
    reset_inactivity_timer();
    uart_test_dirty = true;
    uart_test_device = lv_dropdown_get_selected(ui_Setting_Dropdown_UartDevice);
    if (ui_Setting_Textarea_TextAreaUartLog) {
        lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x808080), 0);
        lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, 
            uart_test_active ? "Параметры изменены. Нажмите для теста" : "Кликните для теста");
    }
}

void SpeedChange(lv_event_t * e) {
    reset_inactivity_timer();
    uart_test_dirty = true;
    char buf[16];
    lv_dropdown_get_selected_str(ui_Setting_Dropdown_SpeedUart, buf, sizeof(buf));
    uart_test_baud = atoi(buf);
    if (ui_Setting_Textarea_TextAreaUartLog) {
        lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x808080), 0);
        lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, 
            uart_test_active ? "Параметры изменены. Нажмите для теста" : "Кликните для теста");
    }
}

void RXChangeDot(lv_event_t * e) {
    reset_inactivity_timer();
    uart_test_dirty = true;
    char buf[16];
    lv_dropdown_get_selected_str(ui_Setting_Dropdown_RXList, buf, sizeof(buf));
    uart_test_rx = atoi(buf);
    if (ui_Setting_Textarea_TextAreaUartLog) {
        lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x808080), 0);
        lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, 
            uart_test_active ? "Параметры изменены. Нажмите для теста" : "Кликните для теста");
    }
}

void TXChangeDot(lv_event_t * e) {
    reset_inactivity_timer();
    uart_test_dirty = true;
    char buf[16];
    lv_dropdown_get_selected_str(ui_Setting_Dropdown_TXList, buf, sizeof(buf));
    uart_test_tx = atoi(buf);
    if (ui_Setting_Textarea_TextAreaUartLog) {
        lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x808080), 0);
        lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, 
            uart_test_active ? "Параметры изменены. Нажмите для теста" : "Кликните для теста");
    }
}

void OnTestScreenUart(lv_event_t * e) {
    reset_inactivity_timer();
    if (uart_test_active) {
        bool success = uart_test_success;
        bool dirty = uart_test_dirty;
        uart_test_stop();

        if (success && !dirty) {
            CarData *data = CarData_Get();
            if (data) {
                CarData_Lock(10);
                if (uart_test_device == 0) {
                    data->arduino_rx_pin = uart_test_rx;
                    data->arduino_tx_pin = uart_test_tx;
                    data->arduino_baud_rate = uart_test_baud;
                    data->arduino_configured = true;
                } else if (uart_test_device == 1) {
                    data->gw_rx_pin = uart_test_rx;
                    data->gw_tx_pin = uart_test_tx;
                    data->gw_baud_rate = uart_test_baud;
                    data->gw_configured = true;
                } else {
                    data->gps_rx_pin = uart_test_rx;
                    data->gps_tx_pin = uart_test_tx;
                    data->gps_baud_rate = uart_test_baud;
                    data->gps_configured = true;
                }
                CarData_Unlock();
                fiona_core_save_car_data_to_nvs();
            }
            if (ui_Setting_Textarea_TextAreaUartLog) {
                lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x00FF00), 0);
                lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, "Сохранено");
            }
        } else if (dirty) {
            uart_test_start();
        } else {
            if (ui_Setting_Textarea_TextAreaUartLog) {
                lv_obj_set_style_text_color(ui_Setting_Textarea_TextAreaUartLog, lv_color_hex(0x808080), 0);
                lv_textarea_set_text(ui_Setting_Textarea_TextAreaUartLog, "Нет данных. Тест остановлен");
            }
        }
    } else {
        uart_test_start();
    }
}