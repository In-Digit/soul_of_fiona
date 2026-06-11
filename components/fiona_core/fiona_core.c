/**
 * @file fiona_core.c
 * @brief Диспетчер и инициализация ядра Фионы.
 *
 * Добавлена переменная engine_off_seconds, глобальный доступ к ней,
 * обработчик загрузки дашборда (скрытие слоя климата),
 * при загрузке дашборда инициализируются наблюдатели и таймеры.
 * Добавлена проверка размера структуры CarData при загрузке из NVS.
 * Теперь здесь же стартует фоновая задача опроса устройств (fiona_io_poller).
 */

#include "fiona_core.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "sd_utils.h"
#include "fiona_soul.h"
#include "fiona_brain.h"
#include "config_manager.h"
#include "rx8025_rtc.h"
#include "esp_log.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "bsp/esp-bsp.h"
#include "driver/uart.h"
#include "fiona_io_poller.h"   // <-- добавлено

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

// Глобальная переменная engine_off_seconds
uint32_t engine_off_seconds = 0;
uint32_t inactivity_seconds = 0;
uint32_t boot_time = 0;
// Внешние объекты экранов
extern lv_obj_t * ui_Screen_DashBoard;
extern lv_obj_t * ui_DashBoard_Container_ContainerClimate;
extern lv_obj_t * ui_DashBoard_Label_FionaSpeachLabelDash;

// Прототип обработчика события загрузки дашборда
static void dashboard_screen_load_cb(lv_event_t * e);

// -------------------- Инициализация --------------------
static void fiona_core_init_timer_cb(lv_timer_t *t) {
    fiona_animations_start_cold_boot();
    lv_timer_del(t);
}

void fiona_core_init(void) {
    CarData *data = CarData_Get();
    if (!data) return;

    // Инициализация внешнего RTC (не критично для работы)
    esp_err_t rtc_ret = rtc_init();
    if (rtc_ret != ESP_OK) {
        ESP_LOGW(TAG, "RTC initialization failed (will rely on gateway time)");
    }

    // Если системное время ещё не было установлено, пробуем восстановить из RTC
    if (!data->time_received_this_boot && rtc_ret == ESP_OK) {
        rtc_time_t rtc_time;
        if (rtc_get_time(&rtc_time) == ESP_OK) {
            struct tm tm;
            memset(&tm, 0, sizeof(tm));
            tm.tm_year = rtc_time.year - 1900;
            tm.tm_mon  = rtc_time.mon - 1;
            tm.tm_mday = rtc_time.day;
            tm.tm_hour = rtc_time.hour;
            tm.tm_min  = rtc_time.min;
            tm.tm_sec  = rtc_time.sec;
            time_t rtc_unix = mktime(&tm);
            if (rtc_unix > 0) {
                struct timeval tv;
                tv.tv_sec = rtc_unix;
                tv.tv_usec = 0;
                settimeofday(&tv, NULL);
                CarData_Lock(10);
                data->systemTime = (uint32_t)rtc_unix;
                CarData_Unlock();
                ESP_LOGI(TAG, "System time restored from RTC");
                // Сохраняем время старта для расчёта uptime
                boot_time = (uint32_t)rtc_unix;
            }
        }
    }

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
    if (config_load_from_sd(data)) {
        ESP_LOGI(TAG, "CarData loaded from SD card, updating NVS...");
        fiona_core_save_car_data_to_nvs();
    } else {
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

    // Обнаружение шлюза и настройка порта Arduino
    int gw_port = uart_discover_gateway();
    if (gw_port >= 0) {
        ESP_LOGI(TAG, "Gateway found on UART%d", gw_port);
    } else {
        ESP_LOGW(TAG, "Gateway not found, continuing without gateway");
    }
    if (uart_get_arduino_port() < 0) {
        uart_set_arduino_port(UART_NUM_2);
    }

    fiona_soul_init();
    fiona_brain_init();

    // Подписываемся на событие загрузки дашборда
    if (ui_Screen_DashBoard) {
        lv_obj_add_event_cb(ui_Screen_DashBoard, dashboard_screen_load_cb, LV_EVENT_SCREEN_LOADED, NULL);
    }

    // Запрашиваем текущий tempOffset у Arduino, если она жива
    if (uart_is_arduino_alive()) {
        uart_send_to_arduino(MSG_TEMP_OFFSET_GET, NULL, 0);
        CarData_Lock(10);
        data->arduino_offset_pending = true;
        CarData_Unlock();
    }

    // Запрашиваем стиль вождения у шлюза
    if (uart_is_gateway_alive()) {
        uart_send_to_gateway(MSG_REQ_DRIVING_STYLE, NULL, 0);
    }

    // Запускаем фоновую задачу опроса устройств
    fiona_io_poller_start();   // <-- добавлено

    lv_timer_create(fiona_core_init_timer_cb, 0, NULL);
}

// -------------------- Загрузка дашборда --------------------
static void dashboard_screen_load_cb(lv_event_t * e) {
    if (ui_DashBoard_Container_ContainerClimate) {
        lv_obj_add_flag(ui_DashBoard_Container_ContainerClimate, LV_OBJ_FLAG_HIDDEN);
    }
    fiona_observers_subscribe_all();
    fiona_observers_init_widgets();
    fiona_background_init_timers();
    fiona_core_refresh_dashboard();
}

void fiona_core_dashboard_on_load(void) {
}

void fiona_core_refresh_dashboard(void) {
    if (clock_timer) lv_timer_reset(clock_timer);
}

void fiona_core_request_internet_time(void) {
    CarData *data = CarData_Get();
    if (data) {
        CarData_Lock(10);
        data->time_set_manually = false;
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
    if (nvs_open("fiona", NVS_READONLY, &handle) != ESP_OK) {
        return;
    }

    CarData *data = CarData_Get();
    if (!data) {
        nvs_close(handle);
        return;
    }

    size_t stored_size = 0;
    esp_err_t err = nvs_get_blob(handle, "cardata", NULL, &stored_size);
    if (err == ESP_OK && stored_size != CarData_get_size()) {
        ESP_LOGW(TAG, "CarData size mismatch (NVS: %d, current: %d). Erasing NVS and SD config.",
                 stored_size, CarData_get_size());
        nvs_close(handle);
        nvs_flash_erase();
        nvs_flash_init();
        remove("/sdcard/fiona/config.json");
        return;
    }

    size_t size = sizeof(CarData);
    if (nvs_get_blob(handle, "cardata", data, &size) == ESP_OK) {
        ESP_LOGI(TAG, "CarData loaded from NVS");
    }
    nvs_close(handle);
}

void fiona_core_load_car_data_from_nvs(void) {
    load_car_data_from_nvs();
}