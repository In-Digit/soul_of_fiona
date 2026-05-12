/**
 * @file fiona_core.c
 * @brief Диспетчер и инициализация ядра Фионы.
 */

#include "fiona_core.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "sd_utils.h"
#include "fiona_soul.h"
#include "fiona_brain.h"
#include "config_manager.h"
#include "esp_log.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "FIONA_CORE";
static void clock_color_reset_timer(lv_timer_t *t);
// Внешние функции из других модулей ядра
void fiona_background_init_timers(void);
void fiona_background_start(void);
void fiona_animations_start_cold_boot(void);
void fiona_observers_subscribe_all(void);
void fiona_observers_init_widgets(void);

// -------------------- Субъекты LVGL --------------------
lv_subject_t subject_speed;
lv_subject_t subject_rpm;
lv_subject_t subject_bat;
lv_subject_t subject_fuel;
lv_subject_t subject_temp;
lv_subject_t subject_odo;
lv_subject_t subject_range;
lv_subject_t subject_lph;
lv_subject_t subject_trip_time;
lv_subject_t subject_trip_pause;
lv_subject_t subject_trip_fuel;
lv_subject_t subject_trip_dist;
lv_subject_t subject_clock_color;
lv_subject_t subject_dash_message;
lv_subject_t subject_adc_light;
lv_subject_t subject_throttle;
lv_subject_t subject_accel;

// -------------------- Таймеры --------------------
lv_timer_t *poll_timer = NULL;
lv_timer_t *clock_timer = NULL;
bool screensaver_active = false;

// -------------------- Инициализация --------------------
static void fiona_core_init_timer_cb(lv_timer_t *t) {
    fiona_animations_start_cold_boot();
    lv_timer_del(t);
}

void fiona_core_init(void) {
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(1000);
    lv_subject_init_int(&subject_speed, 0);
    lv_subject_init_int(&subject_rpm, 0);
    lv_subject_init_int(&subject_bat, 0);
    lv_subject_init_int(&subject_fuel, 0);
    lv_subject_init_int(&subject_temp, 0);
    lv_subject_init_int(&subject_odo, 0);
    lv_subject_init_int(&subject_range, 0);
    lv_subject_init_int(&subject_lph, 0);
    lv_subject_init_int(&subject_trip_time, 0);
    lv_subject_init_int(&subject_trip_pause, 0);
    lv_subject_init_int(&subject_trip_fuel, 0);
    lv_subject_init_int(&subject_trip_dist, 0);
    lv_subject_init_int(&subject_clock_color, 0);
    lv_subject_init_pointer(&subject_dash_message, (void*)"");
    lv_subject_init_int(&subject_adc_light, 0);
    lv_subject_init_int(&subject_throttle, 0);
    lv_subject_init_int(&subject_accel, 0);
    CarData_Unlock();

    bsp_display_brightness_init();
    bsp_display_brightness_set(100);

    // --- Приоритет: SD-карта > NVS ---
    // Сначала пробуем загрузить настройки с SD-карты.
    // Если удалось – применяем их и сразу же сохраняем в NVS, чтобы внутренняя память
    // всегда соответствовала карте.
    if (config_load_from_sd(data)) {
        ESP_LOGI(TAG, "CarData loaded from SD card, updating NVS...");
        fiona_core_save_car_data_to_nvs();   // синхронизируем NVS с картой
    } else {
        // SD-карта не доступна или config.json повреждён – загружаем из NVS
        fiona_core_load_car_data_from_nvs();
    }

    // Защита от старого NVS-блоба без полей яркости
    CarData_Lock(1000);
    if (data->backlight_brightness == 0) data->backlight_brightness = 80;
    if (data->backlight_min_brightness == 0) data->backlight_min_brightness = 5;
    if (data->light_threshold_dark == 0 && data->light_threshold_bright == 0) {
        data->light_threshold_dark = 20;
        data->light_threshold_bright = 80;
    }
    CarData_Unlock();

    fiona_soul_init();
    fiona_brain_init();

    lv_timer_create(fiona_core_init_timer_cb, 0, NULL);
}

// -------------------- Загрузка дашборда --------------------
void fiona_core_dashboard_on_load(void) {
    fiona_observers_subscribe_all();
    fiona_observers_init_widgets();
    fiona_background_init_timers();
    fiona_core_refresh_dashboard();
}

void fiona_core_refresh_dashboard(void) {
    if (clock_timer) lv_timer_reset(clock_timer);
}

// -------------------- Интернет-время --------------------
void fiona_core_request_internet_time(void) {
    CarData *data = CarData_Get();
    if (data) {
        CarData_Lock(10);
        data->time_set_manually = false;   // сбрасываем ручной приоритет
        CarData_Unlock();
    }
    lv_subject_set_int(&subject_clock_color, 1);
    uart_send_to_gateway(MSG_INTERNET_SYNC, NULL, 0);
    uart_send_to_gateway(MSG_REQ_TIME, NULL, 0);
    lv_timer_create(clock_color_reset_timer, 3000, NULL);
}

static void clock_color_reset_timer(lv_timer_t *t) {
    lv_subject_set_int(&subject_clock_color, 0);
    lv_timer_del(t);
}

void fiona_core_set_clock_color(int color) {
    lv_subject_set_int(&subject_clock_color, color);
}

// -------------------- NVS --------------------
static void save_car_data_to_nvs(void) {
    nvs_handle_t handle;
    if (nvs_open("fiona", NVS_READWRITE, &handle) == ESP_OK) {
        CarData *data = CarData_Get();
        if (data) {
            CarData_Lock(100);
            nvs_set_blob(handle, "cardata", data, sizeof(CarData));
            nvs_commit(handle);
            CarData_Unlock();
        }
        nvs_close(handle);
    }
    config_save_to_sd(CarData_Get());
    fiona_brain_save_state();
}

void fiona_core_save_car_data_to_nvs(void) {
    save_car_data_to_nvs();
}

static void load_car_data_from_nvs(void) {
    nvs_handle_t handle;
    if (nvs_open("fiona", NVS_READONLY, &handle) == ESP_OK) {
        CarData *data = CarData_Get();
        size_t size = sizeof(CarData);
        if (data && nvs_get_blob(handle, "cardata", data, &size) == ESP_OK) {
            ESP_LOGI(TAG, "CarData loaded from NVS");
        }
        nvs_close(handle);
    }
}

void fiona_core_load_car_data_from_nvs(void) {
    load_car_data_from_nvs();
}