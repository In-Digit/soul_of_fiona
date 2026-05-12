/**
 * @file fiona_brain.c
 * @brief Движок правил аналитики Фионы.
 *
 * Загружает правила из /sdcard/fiona/rules.json либо создаёт его из вшитых дефолтов.
 * В каждом цикле применяет правила к CarData и обновляет FionaState.
 */

#include "fiona_brain.h"
#include "CarData.h"
#include "fiona_phrase_loader.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "FIONA_BRAIN";

/* --------------------------------------------------------------------------
 * Вшитый JSON правил (дефолт)
 * -------------------------------------------------------------------------- */
static const char* BUILTIN_RULES = 
    "{"
    "  \"version\": 1,"
    "  \"rules\": ["
    "    {"
    "      \"id\": \"thermal_normal\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"coolant_temp\", \"gte\": 40 },"
    "          { \"sensor\": \"coolant_temp\", \"lt\": 105 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"thermal_state\": 1 } }"
    "    },"
    "    {"
    "      \"id\": \"thermal_warm\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"coolant_temp\", \"gte\": 105 },"
    "          { \"sensor\": \"coolant_temp\", \"lt\": 115 },"
    "          { \"sensor\": \"rpm\", \"gt\": 400 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"thermal_state\": 2, \"event_trigger\": \"temp_warm\" } }"
    "    },"
    "    {"
    "      \"id\": \"overheat\","
    "      \"condition\": { \"sensor\": \"coolant_temp\", \"gte\": 115 },"
    "      \"action\": { \"set_state\": { \"thermal_state\": 3, \"tone\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"fuel_low\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"fuel_pct\", \"lt\": 25 },"
    "          { \"sensor\": \"fuel_pct\", \"gte\": 10 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"fuel_state\": 1, \"event_trigger\": \"fuel_low\" } }"
    "    },"
    "    {"
    "      \"id\": \"fuel_critical\","
    "      \"condition\": { \"sensor\": \"fuel_pct\", \"lt\": 10 },"
    "      \"action\": { \"set_state\": { \"fuel_state\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"batt_low\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"batt_voltage\", \"lt\": 12.0 },"
    "          { \"sensor\": \"batt_voltage\", \"gte\": 11.5 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"batt_state\": 1, \"event_trigger\": \"batt_low\" } }"
    "    },"
    "    {"
    "      \"id\": \"batt_critical\","
    "      \"condition\": { \"sensor\": \"batt_voltage\", \"lt\": 11.5 },"
    "      \"action\": { \"set_state\": { \"batt_state\": 2, \"event_trigger\": \"batt_critical\", \"tone\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"batt_over\","
    "      \"condition\": { \"sensor\": \"batt_voltage\", \"gt\": 15.0 },"
    "      \"action\": { \"set_state\": { \"batt_state\": 3, \"event_trigger\": \"batt_over\", \"tone\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"rpm_sport\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"rpm\", \"gt\": 5000 },"
    "          { \"sensor\": \"is_alone\", \"eq\": true }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"event_trigger\": \"rpm_sport\", \"tone\": 1 } }"
    "    },"
    "    {"
    "      \"id\": \"rpm_active\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"rpm\", \"gt\": 4000 },"
    "          { \"sensor\": \"is_alone\", \"eq\": true }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"event_trigger\": \"rpm_active\", \"tone\": 0 } }"
    "    },"
    "    {"
    "      \"id\": \"rpm_family_high\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"rpm\", \"gt\": 3500 },"
    "          { \"sensor\": \"is_alone\", \"eq\": false }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"event_trigger\": \"rpm_family_high\", \"tone\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"rpm_family_calm\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"rpm\", \"gte\": 1500 },"
    "          { \"sensor\": \"rpm\", \"lte\": 2500 },"
    "          { \"sensor\": \"is_alone\", \"eq\": false }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"event_trigger\": \"rpm_family_calm\", \"tone\": 0 } }"
    "    },"
    "    {"
    "      \"id\": \"alone_detect\","
    "      \"condition\": { \"sensor\": \"aggressive_count_10min\", \"gte\": 3 },"
    "      \"action\": { \"set_state\": { \"is_alone\": true } }"
    "    },"
    "    {"
    "      \"id\": \"alone_reset\","
    "      \"condition\": { \"sensor\": \"aggressive_count_10min\", \"lt\": 1 },"
    "      \"action\": { \"set_state\": { \"is_alone\": false } }"
    "    },"
    "    {"
    "      \"id\": \"highway_enter\","
    "      \"condition\": { \"sensor\": \"speed\", \"gte\": 90 },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 3 } }"
    "    },"
    "    {"
    "      \"id\": \"highway_exit\","
    "      \"condition\": { \"sensor\": \"speed\", \"eq\": 0 },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 0 } }"
    "    },"
    "    {"
    "      \"id\": \"long_trip_1h\","
    "      \"condition\": { \"sensor\": \"trip_duration_min\", \"gte\": 60 },"
    "      \"action\": { \"set_state\": { \"long_trip\": true } }"
    "    },"
    "    {"
    "      \"id\": \"long_trip_2h\","
    "      \"condition\": { \"sensor\": \"trip_duration_min\", \"gte\": 120 },"
    "      \"action\": { \"set_state\": { \"very_long_trip\": true } }"
    "    },"
    "    {"
    "      \"id\": \"stop_count_traffic\","
    "      \"condition\": { \"sensor\": \"stop_count_5min\", \"gt\": 4 },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"stop_count_urban\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"stop_count_5min\", \"gte\": 2 },"
    "          { \"sensor\": \"stop_count_5min\", \"lte\": 4 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 1 } }"
    "    },"
    "    {"
    "      \"id\": \"stop_count_calm\","
    "      \"condition\": { \"sensor\": \"stop_count_5min\", \"lt\": 2 },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 0 } }"
    "    },"
    "    {"
    "      \"id\": \"rush_hour_morning_weekday\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"is_weekday\", \"eq\": true },"
    "          { \"sensor\": \"hour\", \"gte\": 7 },"
    "          { \"sensor\": \"hour\", \"lt\": 9 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"rush_hour_evening_weekday\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"is_weekday\", \"eq\": true },"
    "          { \"sensor\": \"hour\", \"gte\": 17 },"
    "          { \"sensor\": \"hour\", \"lt\": 19 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"rush_hour_saturday_morning\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"is_saturday\", \"eq\": true },"
    "          { \"sensor\": \"hour\", \"gte\": 7 },"
    "          { \"sensor\": \"hour\", \"lt\": 9 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 2 } }"
    "    },"
    "    {"
    "      \"id\": \"rush_hour_sunday_evening\","
    "      \"condition\": {"
    "        \"and\": ["
    "          { \"sensor\": \"is_sunday\", \"eq\": true },"
    "          { \"sensor\": \"hour\", \"gte\": 19 },"
    "          { \"sensor\": \"hour\", \"lt\": 22 }"
    "        ]"
    "      },"
    "      \"action\": { \"set_state\": { \"driving_mode\": 2 } }"
    "    }"
    "  ]"
    "}";

/* --------------------------------------------------------------------------
 * Глобальное состояние
 * -------------------------------------------------------------------------- */
static FionaState g_fiona_state;
static cJSON *g_rules_root = NULL;

/* --------------------------------------------------------------------------
 * Внутренние прототипы
 * -------------------------------------------------------------------------- */
static bool eval_condition(cJSON *cond);
static void apply_action(cJSON *action);
static float get_sensor_value(const char *name);
static void save_rules_to_sd(const char *json_content);
static bool load_rules_from_sd(void);

/* --------------------------------------------------------------------------
 * Инициализация
 * -------------------------------------------------------------------------- */
void fiona_brain_init(void) {
    memset(&g_fiona_state, 0, sizeof(g_fiona_state));
    g_fiona_state.initialized = true;
    g_fiona_state.driving_mode = 0; // CALM

    if (!load_rules_from_sd()) {
        save_rules_to_sd(BUILTIN_RULES);
        load_rules_from_sd();
    }

    fiona_brain_load_state();
    ESP_LOGI(TAG, "Brain initialized");
}

/* --------------------------------------------------------------------------
 * Цикл обновления
 * -------------------------------------------------------------------------- */
void fiona_brain_update(void) {
    if (!g_rules_root || !g_fiona_state.initialized) return;

    CarData *data = CarData_Get();
    if (!data) return;

    // Сброс триггера
    g_fiona_state.event_trigger[0] = '\0';
    g_fiona_state.tone = FIONA_TONE_NEUTRAL;
    g_fiona_state.driving_mode = 0; // по умолчанию CALM

    cJSON *rules = cJSON_GetObjectItem(g_rules_root, "rules");
    if (!cJSON_IsArray(rules)) return;

    cJSON *rule = NULL;
    cJSON_ArrayForEach(rule, rules) {
        cJSON *cond = cJSON_GetObjectItem(rule, "condition");
        if (eval_condition(cond)) {
            cJSON *action = cJSON_GetObjectItem(rule, "action");
            apply_action(action);
        }
    }
}

FionaState* fiona_brain_get_state(void) {
    return &g_fiona_state;
}

/* --------------------------------------------------------------------------
 * NVS
 * -------------------------------------------------------------------------- */
void fiona_brain_save_state(void) {
    nvs_handle_t handle;
    if (nvs_open("fiona", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "brainstate", &g_fiona_state, sizeof(FionaState));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

void fiona_brain_load_state(void) {
    nvs_handle_t handle;
    if (nvs_open("fiona", NVS_READONLY, &handle) == ESP_OK) {
        size_t size = sizeof(FionaState);
        nvs_get_blob(handle, "brainstate", &g_fiona_state, &size);
        nvs_close(handle);
    }
}

void fiona_brain_reload_rules(void) {
    if (g_rules_root) {
        cJSON_Delete(g_rules_root);
        g_rules_root = NULL;
    }
    if (!load_rules_from_sd()) {
        save_rules_to_sd(BUILTIN_RULES);
        load_rules_from_sd();
    }
    ESP_LOGI(TAG, "Rules reloaded");
}

/* --------------------------------------------------------------------------
 * Загрузка / сохранение файла правил
 * -------------------------------------------------------------------------- */
static bool load_rules_from_sd(void) {
    const char *path = "/sdcard/fiona/rules.json";
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGW(TAG, "rules.json not found");
        return false;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return false; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    g_rules_root = cJSON_Parse(buf);
    free(buf);
    if (!g_rules_root) {
        ESP_LOGE(TAG, "Failed to parse rules.json");
        return false;
    }

    ESP_LOGI(TAG, "Rules loaded from SD");
    return true;
}

static void save_rules_to_sd(const char *json_content) {
    mkdir("/sdcard/fiona", 0755);
    const char *path = "/sdcard/fiona/rules.json";
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot create rules.json");
        return;
    }
    fputs(json_content, f);
    fclose(f);
    ESP_LOGI(TAG, "Default rules saved to SD");
}

/* --------------------------------------------------------------------------
 * Вычисление условий
 * -------------------------------------------------------------------------- */
static bool eval_condition(cJSON *cond) {
    if (!cond) return false;

    // Безопасно получаем имя сенсора, если оно есть
    cJSON *sensor_item = cJSON_GetObjectItem(cond, "sensor");
    if (cJSON_IsString(sensor_item)) {
        const char *sensor = sensor_item->valuestring;
        float val = get_sensor_value(sensor);
        cJSON *eq  = cJSON_GetObjectItem(cond, "eq");
        cJSON *gt  = cJSON_GetObjectItem(cond, "gt");
        cJSON *gte = cJSON_GetObjectItem(cond, "gte");
        cJSON *lt  = cJSON_GetObjectItem(cond, "lt");
        cJSON *lte = cJSON_GetObjectItem(cond, "lte");
        if (eq  && val != (float)eq->valuedouble)  return false;
        if (gt  && val <= (float)gt->valuedouble)  return false;
        if (gte && val <  (float)gte->valuedouble) return false;
        if (lt  && val >= (float)lt->valuedouble)  return false;
        if (lte && val >  (float)lte->valuedouble) return false;
        return true;
    }

    cJSON *and_cond = cJSON_GetObjectItem(cond, "and");
    if (cJSON_IsArray(and_cond)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, and_cond) {
            if (!eval_condition(item)) return false;
        }
        return true;
    }

    cJSON *or_cond = cJSON_GetObjectItem(cond, "or");
    if (cJSON_IsArray(or_cond)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, or_cond) {
            if (eval_condition(item)) return true;
        }
        return false;
    }

    cJSON *not_cond = cJSON_GetObjectItem(cond, "not");
    if (not_cond) return !eval_condition(not_cond);

    return false;
}
/* --------------------------------------------------------------------------
 * Применение действия
 * -------------------------------------------------------------------------- */
static void apply_action(cJSON *action) {
    cJSON *set = cJSON_GetObjectItem(action, "set_state");
    if (!cJSON_IsObject(set)) return;

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, set) {
        const char *key = item->string;
        if (strcmp(key, "thermal_state") == 0) {
            g_fiona_state.thermal_state = (uint8_t)item->valueint;
        } else if (strcmp(key, "fuel_state") == 0) {
            g_fiona_state.fuel_state = (uint8_t)item->valueint;
        } else if (strcmp(key, "batt_state") == 0) {
            g_fiona_state.batt_state = (uint8_t)item->valueint;
        } else if (strcmp(key, "is_alone") == 0) {
            g_fiona_state.is_alone = (bool)item->valueint;
        } else if (strcmp(key, "event_trigger") == 0) {
            strncpy(g_fiona_state.event_trigger, item->valuestring, sizeof(g_fiona_state.event_trigger) - 1);
        } else if (strcmp(key, "tone") == 0) {
            g_fiona_state.tone = (uint8_t)item->valueint;
        } else if (strcmp(key, "driving_mode") == 0) {
            g_fiona_state.driving_mode = (uint8_t)item->valueint;
        } else if (strcmp(key, "long_trip") == 0) {
            g_fiona_state.long_trip = (bool)item->valueint;
        } else if (strcmp(key, "very_long_trip") == 0) {
            g_fiona_state.very_long_trip = (bool)item->valueint;
        }
    }
}

/* --------------------------------------------------------------------------
 * Получение значения «сенсора»
 * -------------------------------------------------------------------------- */
static float get_sensor_value(const char *name) {
    CarData *data = CarData_Get();
    if (!data) return 0.0f;

    if (strcmp(name, "coolant_temp") == 0) return (float)data->tempValue;
    if (strcmp(name, "rpm") == 0)           return (float)data->rpmValue;
    if (strcmp(name, "fuel_pct") == 0)      return data->fuelValue;
    if (strcmp(name, "batt_voltage") == 0)  return data->batValue;
    if (strcmp(name, "is_alone") == 0)      return g_fiona_state.is_alone ? 1.0f : 0.0f;
    if (strcmp(name, "aggressive_count_10min") == 0) {
        return data->throttlePos > 80 ? 5.0f : 0.0f;
    }

    // Новые сенсоры
    if (strcmp(name, "speed") == 0)          return (float)data->speedValue;
    if (strcmp(name, "trip_duration_min") == 0) {
        return g_fiona_state.trip_duration_sec / 60.0f;
    }
    if (strcmp(name, "stop_count_5min") == 0) return (float)g_fiona_state.stop_count_5min;
    if (strcmp(name, "hour") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        return (float)t->tm_hour;
    }
    if (strcmp(name, "is_weekday") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        return (t->tm_wday >= 1 && t->tm_wday <= 5) ? 1.0f : 0.0f;
    }
    if (strcmp(name, "is_saturday") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        return (t->tm_wday == 6) ? 1.0f : 0.0f;
    }
    if (strcmp(name, "is_sunday") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        return (t->tm_wday == 0) ? 1.0f : 0.0f;
    }

    return 0.0f;
}