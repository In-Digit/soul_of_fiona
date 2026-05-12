/**
 * @file fiona_animations.c
 * @brief Сценарии холодного старта, скринсейвера, пробуждения и цветовые фильтры.
 */

#include "fiona_animations.h"
#include "fiona_core.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "fiona_soul.h"
#include "fiona_phrase_loader.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "FIONA_ANIM";

// -------------------- Внешние UI-объекты --------------------
extern lv_obj_t * ui_Screen_DashBoard;
extern lv_obj_t * ui_Screen_SplashScreen;
extern lv_obj_t * ui_SplashScreen_Image_Logo;
extern lv_obj_t * ui_SplashScreen_Label_Clock;
extern lv_obj_t * ui_SplashScreen_Label_FionaSpeachLabel;
extern lv_obj_t * ui_DashBoard_Image_Backgrownd;
extern lv_obj_t * ui_DashBoard_Image_TripImg;
extern lv_obj_t * ui_DashBoard_Image_BlueRing;

// -------------------- Глобальные переменные --------------------
extern bool screensaver_active;
static bool screensaver_clock_anim_active = false;

// Цветовые фильтры
static lv_color_filter_dsc_t bg_filter_dsc;
static uint8_t* bg_theme_ptr = NULL;

static lv_color_filter_dsc_t trip_filter_dsc;
static uint8_t trip_color_state = 0; // 0=исх, 1=зел, 2=синий

static lv_color_filter_dsc_t ring_filter_dsc;
static bool ring_internet_available = false;

// Прототипы
static void anim_set_opa_cb(void *var, int32_t val);
static void logo_fade_in_done(lv_anim_t *a);
static void boot_log_timer_cb(lv_timer_t *t);
static void wait_for_connection_timer(lv_timer_t *t);
static void logo_fade_out_done(lv_anim_t *a);
static void greeting_hold_timer(lv_timer_t *t);
static void screensaver_phrase_fade_in_complete(lv_anim_t *a);
static void screensaver_phrase_hold_timer_cb(lv_timer_t *t);
static void screensaver_phrase_fade_out_complete(lv_anim_t *a);
static void clock_fade_out_complete(lv_anim_t *a);
static void wake_phrase_fade_in_complete(lv_anim_t *a);
static void wake_phrase_hold_timer_cb(lv_timer_t *t);

/* -------------------------------------------------------------------------- */
/* Холодный старт                                                             */
/* -------------------------------------------------------------------------- */
static const char *boot_phrases[] = {
    "Инициализация SD-карты...",
    "Загрузка данных...",
    "Подключение к нервной системе...",
    "Подключение к системе обратной связи...",
    "Синхронизация времени..."
};
static int boot_phrase_count = sizeof(boot_phrases) / sizeof(boot_phrases[0]);
static int current_boot_idx = 0;

void fiona_animations_start_cold_boot(void) {
    lv_disp_load_scr(ui_Screen_SplashScreen);
    lv_obj_add_flag(ui_SplashScreen_Label_Clock, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_SplashScreen_Label_FionaSpeachLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui_SplashScreen_Image_Logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(ui_SplashScreen_Image_Logo, 0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_SplashScreen_Image_Logo);
    lv_anim_set_time(&a, 1500);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_exec_cb(&a, anim_set_opa_cb);
    lv_anim_set_completed_cb(&a, logo_fade_in_done);
    lv_anim_start(&a);
}

static void logo_fade_in_done(lv_anim_t *a) {
    current_boot_idx = 0;
    lv_label_set_text(ui_SplashScreen_Label_FionaSpeachLabel, boot_phrases[current_boot_idx]);
    lv_obj_remove_flag(ui_SplashScreen_Label_FionaSpeachLabel, LV_OBJ_FLAG_HIDDEN);
    lv_timer_create(boot_log_timer_cb, 800, NULL);
}

static void boot_log_timer_cb(lv_timer_t *t) {
    current_boot_idx++;
    if (current_boot_idx < boot_phrase_count) {
        lv_label_set_text(ui_SplashScreen_Label_FionaSpeachLabel, boot_phrases[current_boot_idx]);
        if (current_boot_idx == 3) {
            uart_send_broadcast(MSG_WHO_IS_HERE, NULL, 0);
            uart_send_to_gateway(MSG_REQ_TIME, NULL, 0);
        }
    } else {
        lv_timer_del(t);
        lv_timer_create(wait_for_connection_timer, 500, NULL);
    }
}

static void wait_for_connection_timer(lv_timer_t *t) {
    CarData *data = CarData_Get();
    bool synced = false;
    if (data) {
        CarData_Lock(0);
        synced = data->time_received_this_boot;
        CarData_Unlock();
    }
    if (synced && uart_is_gateway_alive()) {
        lv_timer_del(t);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, ui_SplashScreen_Image_Logo);
        lv_anim_set_time(&a, 1000);
        lv_anim_set_values(&a, 255, 0);
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);
        lv_anim_set_completed_cb(&a, logo_fade_out_done);
        lv_anim_start(&a);
    }
}

static void logo_fade_out_done(lv_anim_t *a) {
    lv_obj_add_flag(ui_SplashScreen_Image_Logo, LV_OBJ_FLAG_HIDDEN);
    FionaPhrase greeting = fiona_soul_get_start_phrase_by_time();
    lv_label_set_text(ui_SplashScreen_Label_FionaSpeachLabel, greeting.text);
    lv_obj_set_style_opa(ui_SplashScreen_Label_FionaSpeachLabel, 255, 0);
    lv_obj_remove_flag(ui_SplashScreen_Label_FionaSpeachLabel, LV_OBJ_FLAG_HIDDEN);
    lv_timer_create(greeting_hold_timer, 1500, NULL);
}

static void greeting_hold_timer(lv_timer_t *t) {
    lv_screen_load_anim(ui_Screen_DashBoard, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
    lv_timer_del(t);
}

/* -------------------------------------------------------------------------- */
/* Скринсейвер                                                                 */
/* -------------------------------------------------------------------------- */
void fiona_core_activate_screensaver(void) {
    if (screensaver_active) return;
    screensaver_active = true;
    fiona_core_save_car_data_to_nvs();
    lv_disp_load_scr(ui_Screen_SplashScreen);
    lv_obj_remove_flag(ui_SplashScreen_Label_Clock, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(ui_SplashScreen_Label_Clock, 128, 0);
    lv_anim_t breath;
    lv_anim_init(&breath);
    lv_anim_set_var(&breath, ui_SplashScreen_Label_Clock);
    lv_anim_set_time(&breath, 2000);
    lv_anim_set_values(&breath, 128, 255);
    lv_anim_set_exec_cb(&breath, anim_set_opa_cb);
    lv_anim_set_playback_time(&breath, 2000);
    lv_anim_set_repeat_count(&breath, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&breath);
    screensaver_clock_anim_active = true;

    const char *sleep_text = phrase_loader_pick("sleep_normal", FIONA_TONE_FUNNY);
    if (!sleep_text || sleep_text[0] == '\0') {
        sleep_text = phrase_loader_pick("sleep_normal", FIONA_TONE_NEUTRAL);
    }
    lv_label_set_text(ui_SplashScreen_Label_FionaSpeachLabel, sleep_text ? sleep_text : "Сплю...");
    lv_obj_remove_flag(ui_SplashScreen_Label_FionaSpeachLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(ui_SplashScreen_Label_FionaSpeachLabel, 0, 0);
    lv_anim_t fade_in;
    lv_anim_init(&fade_in);
    lv_anim_set_var(&fade_in, ui_SplashScreen_Label_FionaSpeachLabel);
    lv_anim_set_time(&fade_in, 500);
    lv_anim_set_values(&fade_in, 0, 255);
    lv_anim_set_exec_cb(&fade_in, anim_set_opa_cb);
    lv_anim_set_completed_cb(&fade_in, screensaver_phrase_fade_in_complete);
    lv_anim_start(&fade_in);
}

static void screensaver_phrase_fade_in_complete(lv_anim_t *a) {
    lv_timer_create(screensaver_phrase_hold_timer_cb, 4000, NULL);
}

static void screensaver_phrase_hold_timer_cb(lv_timer_t *t) {
    lv_anim_t fade_out;
    lv_anim_init(&fade_out);
    lv_anim_set_var(&fade_out, ui_SplashScreen_Label_FionaSpeachLabel);
    lv_anim_set_time(&fade_out, 500);
    lv_anim_set_values(&fade_out, 255, 0);
    lv_anim_set_exec_cb(&fade_out, anim_set_opa_cb);
    lv_anim_set_completed_cb(&fade_out, screensaver_phrase_fade_out_complete);
    lv_anim_start(&fade_out);
    lv_timer_del(t);
}

static void screensaver_phrase_fade_out_complete(lv_anim_t *a) {
    lv_obj_add_flag(ui_SplashScreen_Label_FionaSpeachLabel, LV_OBJ_FLAG_HIDDEN);
}

/* -------------------------------------------------------------------------- */
/* Пробуждение                                                                 */
/* -------------------------------------------------------------------------- */
void fiona_core_deactivate_screensaver(void) {
    if (!screensaver_active) return;
    screensaver_active = false;

    if (screensaver_clock_anim_active) {
        lv_anim_delete(ui_SplashScreen_Label_Clock, anim_set_opa_cb);
        screensaver_clock_anim_active = false;
    }

    lv_anim_t clock_fade;
    lv_anim_init(&clock_fade);
    lv_anim_set_var(&clock_fade, ui_SplashScreen_Label_Clock);
    lv_anim_set_time(&clock_fade, 1000);
    lv_anim_set_values(&clock_fade, lv_obj_get_style_opa(ui_SplashScreen_Label_Clock, 0), 0);
    lv_anim_set_exec_cb(&clock_fade, anim_set_opa_cb);
    lv_anim_set_completed_cb(&clock_fade, clock_fade_out_complete);
    lv_anim_start(&clock_fade);
}

static void clock_fade_out_complete(lv_anim_t *a) {
    lv_obj_add_flag(ui_SplashScreen_Label_Clock, LV_OBJ_FLAG_HIDDEN);

    const char *wake_text = phrase_loader_pick("wake_warm", FIONA_TONE_FUNNY);
    if (!wake_text || wake_text[0] == '\0') {
        wake_text = phrase_loader_pick("wake_cold", FIONA_TONE_FUNNY);
    }
    if (!wake_text || wake_text[0] == '\0') {
        wake_text = phrase_loader_pick("wake_cold", FIONA_TONE_NEUTRAL);
    }
    lv_label_set_text(ui_SplashScreen_Label_FionaSpeachLabel, wake_text ? wake_text : "Просыпаюсь...");
    lv_obj_remove_flag(ui_SplashScreen_Label_FionaSpeachLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(ui_SplashScreen_Label_FionaSpeachLabel, 0, 0);
    lv_anim_t wake_fade;
    lv_anim_init(&wake_fade);
    lv_anim_set_var(&wake_fade, ui_SplashScreen_Label_FionaSpeachLabel);
    lv_anim_set_time(&wake_fade, 500);
    lv_anim_set_values(&wake_fade, 0, 255);
    lv_anim_set_exec_cb(&wake_fade, anim_set_opa_cb);
    lv_anim_set_completed_cb(&wake_fade, wake_phrase_fade_in_complete);
    lv_anim_start(&wake_fade);
}

static void wake_phrase_fade_in_complete(lv_anim_t *a) {
    lv_timer_create(wake_phrase_hold_timer_cb, 3000, NULL);
}

static void wake_phrase_hold_timer_cb(lv_timer_t *t) {
    lv_screen_load_anim(ui_Screen_DashBoard, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
    lv_timer_del(t);
}

/* ========================================================================== */
/*            ЦВЕТОВЫЕ ФИЛЬТРЫ И ТЕМЫ                                          */
/* ========================================================================== */

/* ---------- Фон (синий -> зелёный/красный) ---------- */
static lv_color_t bg_color_filter_cb(const lv_color_filter_dsc_t * dsc, lv_color_t color, lv_opa_t opa)
{
    uint8_t theme = 0;
    if (bg_theme_ptr) theme = *bg_theme_ptr;
    if (theme == 0) return color;

    // RGB888 напрямую
    uint8_t r = color.red;
    uint8_t g = color.green;
    uint8_t b = color.blue;

    // Синий преобладает
    if (b > r && b > g) {
        if (theme == 1) { // зелёный
            color.red   = (b * 10) / 100;
            color.green = (b * 90) / 100;
            color.blue  = (b * 20) / 100;
        } else if (theme == 2) { // красный
            color.red   = (b * 90) / 100;
            color.green = (b * 10) / 100;
            color.blue  = (b * 20) / 100;
        }
    }
    return color;
}

void fiona_animations_apply_bg_theme(uint8_t theme) {
    if (!ui_DashBoard_Image_Backgrownd) return;

    static uint8_t current_theme = 255;
    if (theme == current_theme) return;
    current_theme = theme;
    bg_theme_ptr = &current_theme;

    ESP_LOGI(TAG, "Bg theme changed to %d", theme);

    lv_color_filter_dsc_init(&bg_filter_dsc, bg_color_filter_cb);
    lv_obj_set_style_color_filter_dsc(ui_DashBoard_Image_Backgrownd, &bg_filter_dsc, LV_PART_MAIN);
    lv_obj_invalidate(ui_DashBoard_Image_Backgrownd);
}

/* ---------- TripImg (оранжевый -> зелёный/синий) ---------- */
static lv_color_t tripimg_color_filter_cb(const lv_color_filter_dsc_t * dsc, lv_color_t color, lv_opa_t opa)
{
    // Оранжевый #FFB600 в RGB888: R=255, G=182, B=0
    // Допускаем небольшой разброс
    if (color.red >= 240 && color.green >= 160 && color.green <= 200 && color.blue <= 20) {
        if (trip_color_state == 1) { // зелёный
            color.red = 0;
            color.green = 255;
            color.blue = 0;
        } else if (trip_color_state == 2) { // синий
            color.red = 0;
            color.green = 0;
            color.blue = 255;
        }
    }
    return color;
}

void fiona_animations_set_tripimg_color(uint8_t state) {
    if (!ui_DashBoard_Image_TripImg) return;
    if (state == trip_color_state) return;
    trip_color_state = state;

    ESP_LOGI(TAG, "TripImg color state: %d", state);

    lv_color_filter_dsc_init(&trip_filter_dsc, tripimg_color_filter_cb);
    if (state == 0) {
        lv_obj_set_style_color_filter_dsc(ui_DashBoard_Image_TripImg, NULL, LV_PART_MAIN);
    } else {
        lv_obj_set_style_color_filter_dsc(ui_DashBoard_Image_TripImg, &trip_filter_dsc, LV_PART_MAIN);
    }
    lv_obj_invalidate(ui_DashBoard_Image_TripImg);
}

/* ---------- BlueRing (синий -> зелёный при интернете) ---------- */
static lv_color_t ring_color_filter_cb(const lv_color_filter_dsc_t * dsc, lv_color_t color, lv_opa_t opa)
{
    if (!ring_internet_available) return color;

    uint8_t r = color.red;
    uint8_t g = color.green;
    uint8_t b = color.blue;

    // Синий пиксель
    if (b > r && b > g) {
        color.red   = 0;
        color.green = b;  // сохраняем яркость
        color.blue  = 0;
    }
    return color;
}

void fiona_animations_set_bluering_color(bool internet_available) {
    if (!ui_DashBoard_Image_BlueRing) return;
    if (internet_available == ring_internet_available) return;
    ring_internet_available = internet_available;

    ESP_LOGI(TAG, "Ring internet color: %d", internet_available);

    lv_color_filter_dsc_init(&ring_filter_dsc, ring_color_filter_cb);
    if (!internet_available) {
        lv_obj_set_style_color_filter_dsc(ui_DashBoard_Image_BlueRing, NULL, LV_PART_MAIN);
    } else {
        lv_obj_set_style_color_filter_dsc(ui_DashBoard_Image_BlueRing, &ring_filter_dsc, LV_PART_MAIN);
    }
    lv_obj_invalidate(ui_DashBoard_Image_BlueRing);
}

/* -------------------------------------------------------------------------- */
/* Общая callback-функция прозрачности                                        */
/* -------------------------------------------------------------------------- */
static void anim_set_opa_cb(void *var, int32_t val) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)val, 0);
}