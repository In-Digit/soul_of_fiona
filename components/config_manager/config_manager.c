/**
 * @file config_manager.c
 * @brief Загрузка и сохранение конфигурации CarData в JSON на SD-карте.
 *
 * Обрабатывает настройки интерфейса, пороги, калибровки, а также новые поля:
 *   - climateCalibStartPoint, climateCalibStopPoint, climateCalibNoiseLow, climateCalibNoiseHigh
 *   - climate_target_temp, temp_offset_arduino (опционально)
 *   - trip_force_active (принудительная поездка)
 *   - ручные настройки UART (gw_*, arduino_*, gps_*)
 * Добавлена проверка _struct_size для автоматического сброса при изменении структуры CarData.
 */

#include "config_manager.h"
#include "cJSON.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static const char *TAG = "CONFIG_MANAGER";
#define CONFIG_FILE_PATH "/sdcard/fiona/config.json"

static char *read_file_to_string(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = (char*)malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

// Теперь apply_json_to_car_data принимает готовый cJSON-объект, а не строку
static bool apply_json_to_car_data(CarData *data, cJSON *root) {
    if (!root) return false;

    cJSON *colors = cJSON_GetObjectItem(root, "colors");
    if (cJSON_IsObject(colors)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(colors, "cyan")))       sscanf(item->valuestring, "0x%x", &data->colorCyan);
        if ((item = cJSON_GetObjectItem(colors, "orange")))      sscanf(item->valuestring, "0x%x", &data->colorOrange);
        if ((item = cJSON_GetObjectItem(colors, "red")))         sscanf(item->valuestring, "0x%x", &data->colorRed);
        if ((item = cJSON_GetObjectItem(colors, "green")))       sscanf(item->valuestring, "0x%x", &data->colorGreen);
        if ((item = cJSON_GetObjectItem(colors, "blue")))        sscanf(item->valuestring, "0x%x", &data->colorBlue);
        if ((item = cJSON_GetObjectItem(colors, "yellow")))      sscanf(item->valuestring, "0x%x", &data->colorYellow);
    }

    cJSON *thr = cJSON_GetObjectItem(root, "thresholds");
    if (cJSON_IsObject(thr)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(thr, "fuel_red")))       data->fuelRedThreshold = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "fuel_yellow")))    data->fuelYellowThreshold = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "batt_low")))       data->battLowThreshold = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(thr, "batt_high")))      data->battHighThreshold = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(thr, "temp_cold")))      data->tempCold = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "temp_normal")))    data->tempNormal = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "temp_warm")))      data->tempWarm = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "temp_hot")))       data->tempHot = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_low")))      data->speedLow = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_mid_low")))  data->speedMidLow = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_mid_high"))) data->speedMidHigh = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_high_low"))) data->speedHighLow = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_high_high")))data->speedHighHigh = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_vhigh_low")))data->speedVHighLow = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "speed_vhigh_high")))data->speedVHighHigh = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "rpm_blue_max")))   data->rpmBlueMax = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "rpm_cyan_max")))   data->rpmCyanMax = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "rpm_yellow_max"))) data->rpmYellowMax = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "lph_blue_max")))   data->lphBlueMax = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "lph_cyan_max")))   data->lphCyanMax = item->valueint;
        if ((item = cJSON_GetObjectItem(thr, "lph_yellow_max"))) data->lphYellowMax = item->valueint;
    }

    cJSON *rng = cJSON_GetObjectItem(root, "ranges");
    if (cJSON_IsObject(rng)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(rng, "fuel_min")))       data->fuelMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "fuel_max")))       data->fuelMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "fuel_step")))      data->fuelStep = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "range_min")))      data->rangeMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "range_max")))      data->rangeMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "range_step")))     data->rangeStep = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "batt_min")))       data->battMin = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(rng, "batt_max")))       data->battMax = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(rng, "batt_step")))      data->battStep = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(rng, "temp_min")))       data->tempMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "temp_max")))       data->tempMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "temp_step")))      data->tempStep = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "speed_min")))      data->speedMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "speed_max")))      data->speedMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "speed_step")))     data->speedStep = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "rpm_min")))        data->rpmMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "rpm_max")))        data->rpmMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "rpm_step")))       data->rpmStep = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "trip_m_min")))     data->tripMMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "trip_m_max")))     data->tripMMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "trip_m_step")))    data->tripMStep = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "lph_min")))        data->lphMin = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "lph_max")))        data->lphMax = item->valueint;
        if ((item = cJSON_GetObjectItem(rng, "lph_step")))       data->lphStep = (float)item->valuedouble;
    }

    cJSON *cal = cJSON_GetObjectItem(root, "calibration");
    if (cJSON_IsObject(cal)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(cal, "fuel_price")))             data->fuelPrice = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(cal, "calibration_factor")))     data->fuelCalibrationFactor = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(cal, "screensaver_timeout_sec")))data->screensaver_timeout_sec = item->valueint;
        if ((item = cJSON_GetObjectItem(cal, "trip_auto_stop_timeout"))) data->tripAutoStopTimeout = item->valueint;
    }

    cJSON *clim = cJSON_GetObjectItem(root, "climate");
    if (cJSON_IsObject(clim)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(clim, "fan_setpoint")))    data->fanSetpoint = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(clim, "fan_kp")))          data->fanKp = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(clim, "fan_ki")))          data->fanKi = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(clim, "fan_kd")))          data->fanKd = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(clim, "climate_setpoint")))data->climateSetpoint = (float)item->valuedouble;
    }

    // --------------- КАЛИБРОВОЧНЫЕ ТОЧКИ КЛИМАТА ---------------
    cJSON *calib = cJSON_GetObjectItem(root, "climate_calib");
    if (cJSON_IsObject(calib)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(calib, "start_point")))    data->climateCalibStartPoint = (uint8_t)item->valueint;
        if ((item = cJSON_GetObjectItem(calib, "stop_point")))     data->climateCalibStopPoint = (uint8_t)item->valueint;
        if ((item = cJSON_GetObjectItem(calib, "noise_low")))      data->climateCalibNoiseLow = (uint8_t)item->valueint;
        if ((item = cJSON_GetObjectItem(calib, "noise_high")))     data->climateCalibNoiseHigh = (uint8_t)item->valueint;
    }

    cJSON *light = cJSON_GetObjectItem(root, "light");
    if (cJSON_IsObject(light)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(light, "brightness")))       data->backlight_brightness = item->valueint;
        if ((item = cJSON_GetObjectItem(light, "min_brightness")))   data->backlight_min_brightness = item->valueint;
    }

    // --------------- ПРИНУДИТЕЛЬНАЯ ПОЕЗДКА ---------------
    cJSON *trip = cJSON_GetObjectItem(root, "trip");
    if (cJSON_IsObject(trip)) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(trip, "force_active"))) data->trip_force_active = item->valueint;
    }

    // --------------- РУЧНЫЕ НАСТРОЙКИ UART ---------------
    cJSON *uart = cJSON_GetObjectItem(root, "uart");
    if (cJSON_IsObject(uart)) {
        cJSON *gw = cJSON_GetObjectItem(uart, "gateway");
        if (cJSON_IsObject(gw)) {
            cJSON *item;
            if ((item = cJSON_GetObjectItem(gw, "rx")))   data->gw_rx_pin = item->valueint;
            if ((item = cJSON_GetObjectItem(gw, "tx")))   data->gw_tx_pin = item->valueint;
            if ((item = cJSON_GetObjectItem(gw, "baud"))) data->gw_baud_rate = item->valueint;
            if ((item = cJSON_GetObjectItem(gw, "configured"))) data->gw_configured = item->valueint;
        }

        cJSON *ard = cJSON_GetObjectItem(uart, "arduino");
        if (cJSON_IsObject(ard)) {
            cJSON *item;
            if ((item = cJSON_GetObjectItem(ard, "rx")))   data->arduino_rx_pin = item->valueint;
            if ((item = cJSON_GetObjectItem(ard, "tx")))   data->arduino_tx_pin = item->valueint;
            if ((item = cJSON_GetObjectItem(ard, "baud"))) data->arduino_baud_rate = item->valueint;
            if ((item = cJSON_GetObjectItem(ard, "configured"))) data->arduino_configured = item->valueint;
        }

        cJSON *gps = cJSON_GetObjectItem(uart, "gps");
        if (cJSON_IsObject(gps)) {
            cJSON *item;
            if ((item = cJSON_GetObjectItem(gps, "rx")))   data->gps_rx_pin = item->valueint;
            if ((item = cJSON_GetObjectItem(gps, "tx")))   data->gps_tx_pin = item->valueint;
            if ((item = cJSON_GetObjectItem(gps, "baud"))) data->gps_baud_rate = item->valueint;
            if ((item = cJSON_GetObjectItem(gps, "configured"))) data->gps_configured = item->valueint;
        }
    }

    // Не удаляем root, так как он будет удалён вызывающей функцией
    return true;
}

bool config_load_from_sd(CarData *data) {
    if (!data) return false;
    struct stat st;
    if (stat(CONFIG_FILE_PATH, &st) != 0) {
        ESP_LOGW(TAG, "Config file not found");
        return false;
    }
    char *file_content = read_file_to_string(CONFIG_FILE_PATH);
    if (!file_content) { ESP_LOGE(TAG, "Failed to read config file"); return false; }

    cJSON *root = cJSON_Parse(file_content);
    free(file_content);
    if (!root) { ESP_LOGE(TAG, "Failed to parse JSON"); return false; }

    // Проверяем версию структуры
    cJSON *struct_size_item = cJSON_GetObjectItem(root, "_struct_size");
    if (cJSON_IsNumber(struct_size_item)) {
        int stored_size = struct_size_item->valueint;
        if (stored_size != (int)CarData_get_size()) {
            ESP_LOGW(TAG, "Config struct size mismatch (config: %d, current: %d). Deleting config.",
                     stored_size, (int)CarData_get_size());
            cJSON_Delete(root);
            remove(CONFIG_FILE_PATH);
            return false;
        }
    } else {
        // Поле отсутствует — старый конфиг, тоже удаляем
        ESP_LOGW(TAG, "No _struct_size in config. Deleting old config.");
        cJSON_Delete(root);
        remove(CONFIG_FILE_PATH);
        return false;
    }

    bool ok = apply_json_to_car_data(data, root);
    cJSON_Delete(root);
    if (ok) ESP_LOGI(TAG, "Configuration loaded from SD");
    return ok;
}

static cJSON *car_data_to_json(const CarData *data) {
    cJSON *root = cJSON_CreateObject();
    // Сохраняем размер структуры для проверки совместимости
    cJSON_AddNumberToObject(root, "_struct_size", (int)CarData_get_size());

    cJSON *colors = cJSON_AddObjectToObject(root, "colors");
    char hex[16];
    snprintf(hex, sizeof(hex), "0x%06x", data->colorCyan);   cJSON_AddStringToObject(colors, "cyan", hex);
    snprintf(hex, sizeof(hex), "0x%06x", data->colorOrange); cJSON_AddStringToObject(colors, "orange", hex);
    snprintf(hex, sizeof(hex), "0x%06x", data->colorRed);    cJSON_AddStringToObject(colors, "red", hex);
    snprintf(hex, sizeof(hex), "0x%06x", data->colorGreen);  cJSON_AddStringToObject(colors, "green", hex);
    snprintf(hex, sizeof(hex), "0x%06x", data->colorBlue);   cJSON_AddStringToObject(colors, "blue", hex);
    snprintf(hex, sizeof(hex), "0x%06x", data->colorYellow); cJSON_AddStringToObject(colors, "yellow", hex);

    cJSON *thr = cJSON_AddObjectToObject(root, "thresholds");
    cJSON_AddNumberToObject(thr, "fuel_red", data->fuelRedThreshold);
    cJSON_AddNumberToObject(thr, "fuel_yellow", data->fuelYellowThreshold);
    cJSON_AddNumberToObject(thr, "batt_low", data->battLowThreshold);
    cJSON_AddNumberToObject(thr, "batt_high", data->battHighThreshold);
    cJSON_AddNumberToObject(thr, "temp_cold", data->tempCold);
    cJSON_AddNumberToObject(thr, "temp_normal", data->tempNormal);
    cJSON_AddNumberToObject(thr, "temp_warm", data->tempWarm);
    cJSON_AddNumberToObject(thr, "temp_hot", data->tempHot);
    cJSON_AddNumberToObject(thr, "speed_low", data->speedLow);
    cJSON_AddNumberToObject(thr, "speed_mid_low", data->speedMidLow);
    cJSON_AddNumberToObject(thr, "speed_mid_high", data->speedMidHigh);
    cJSON_AddNumberToObject(thr, "speed_high_low", data->speedHighLow);
    cJSON_AddNumberToObject(thr, "speed_high_high", data->speedHighHigh);
    cJSON_AddNumberToObject(thr, "speed_vhigh_low", data->speedVHighLow);
    cJSON_AddNumberToObject(thr, "speed_vhigh_high", data->speedVHighHigh);
    cJSON_AddNumberToObject(thr, "rpm_blue_max", data->rpmBlueMax);
    cJSON_AddNumberToObject(thr, "rpm_cyan_max", data->rpmCyanMax);
    cJSON_AddNumberToObject(thr, "rpm_yellow_max", data->rpmYellowMax);
    cJSON_AddNumberToObject(thr, "lph_blue_max", data->lphBlueMax);
    cJSON_AddNumberToObject(thr, "lph_cyan_max", data->lphCyanMax);
    cJSON_AddNumberToObject(thr, "lph_yellow_max", data->lphYellowMax);

    cJSON *rng = cJSON_AddObjectToObject(root, "ranges");
    cJSON_AddNumberToObject(rng, "fuel_min", data->fuelMin);
    cJSON_AddNumberToObject(rng, "fuel_max", data->fuelMax);
    cJSON_AddNumberToObject(rng, "fuel_step", data->fuelStep);
    cJSON_AddNumberToObject(rng, "range_min", data->rangeMin);
    cJSON_AddNumberToObject(rng, "range_max", data->rangeMax);
    cJSON_AddNumberToObject(rng, "range_step", data->rangeStep);
    cJSON_AddNumberToObject(rng, "batt_min", data->battMin);
    cJSON_AddNumberToObject(rng, "batt_max", data->battMax);
    cJSON_AddNumberToObject(rng, "batt_step", data->battStep);
    cJSON_AddNumberToObject(rng, "temp_min", data->tempMin);
    cJSON_AddNumberToObject(rng, "temp_max", data->tempMax);
    cJSON_AddNumberToObject(rng, "temp_step", data->tempStep);
    cJSON_AddNumberToObject(rng, "speed_min", data->speedMin);
    cJSON_AddNumberToObject(rng, "speed_max", data->speedMax);
    cJSON_AddNumberToObject(rng, "speed_step", data->speedStep);
    cJSON_AddNumberToObject(rng, "rpm_min", data->rpmMin);
    cJSON_AddNumberToObject(rng, "rpm_max", data->rpmMax);
    cJSON_AddNumberToObject(rng, "rpm_step", data->rpmStep);
    cJSON_AddNumberToObject(rng, "trip_m_min", data->tripMMin);
    cJSON_AddNumberToObject(rng, "trip_m_max", data->tripMMax);
    cJSON_AddNumberToObject(rng, "trip_m_step", data->tripMStep);
    cJSON_AddNumberToObject(rng, "lph_min", data->lphMin);
    cJSON_AddNumberToObject(rng, "lph_max", data->lphMax);
    cJSON_AddNumberToObject(rng, "lph_step", data->lphStep);

    cJSON *cal = cJSON_AddObjectToObject(root, "calibration");
    cJSON_AddNumberToObject(cal, "fuel_price", data->fuelPrice);
    cJSON_AddNumberToObject(cal, "calibration_factor", data->fuelCalibrationFactor);
    cJSON_AddNumberToObject(cal, "screensaver_timeout_sec", data->screensaver_timeout_sec);
    cJSON_AddNumberToObject(cal, "trip_auto_stop_timeout", data->tripAutoStopTimeout);

    cJSON *clim = cJSON_AddObjectToObject(root, "climate");
    cJSON_AddNumberToObject(clim, "fan_setpoint", data->fanSetpoint);
    cJSON_AddNumberToObject(clim, "fan_kp", data->fanKp);
    cJSON_AddNumberToObject(clim, "fan_ki", data->fanKi);
    cJSON_AddNumberToObject(clim, "fan_kd", data->fanKd);
    cJSON_AddNumberToObject(clim, "climate_setpoint", data->climateSetpoint);

    // --------------- КАЛИБРОВОЧНЫЕ ТОЧКИ КЛИМАТА ---------------
    cJSON *calib = cJSON_AddObjectToObject(root, "climate_calib");
    cJSON_AddNumberToObject(calib, "start_point", data->climateCalibStartPoint);
    cJSON_AddNumberToObject(calib, "stop_point", data->climateCalibStopPoint);
    cJSON_AddNumberToObject(calib, "noise_low", data->climateCalibNoiseLow);
    cJSON_AddNumberToObject(calib, "noise_high", data->climateCalibNoiseHigh);

    cJSON *light = cJSON_AddObjectToObject(root, "light");
    cJSON_AddNumberToObject(light, "brightness", data->backlight_brightness);
    cJSON_AddNumberToObject(light, "min_brightness", data->backlight_min_brightness);

    // --------------- ПРИНУДИТЕЛЬНАЯ ПОЕЗДКА ---------------
    cJSON *trip = cJSON_AddObjectToObject(root, "trip");
    cJSON_AddBoolToObject(trip, "force_active", data->trip_force_active);

    // --------------- РУЧНЫЕ НАСТРОЙКИ UART ---------------
    cJSON *uart = cJSON_AddObjectToObject(root, "uart");

    cJSON *gw = cJSON_AddObjectToObject(uart, "gateway");
    cJSON_AddNumberToObject(gw, "rx", data->gw_rx_pin);
    cJSON_AddNumberToObject(gw, "tx", data->gw_tx_pin);
    cJSON_AddNumberToObject(gw, "baud", data->gw_baud_rate);
    cJSON_AddBoolToObject(gw, "configured", data->gw_configured);

    cJSON *ard = cJSON_AddObjectToObject(uart, "arduino");
    cJSON_AddNumberToObject(ard, "rx", data->arduino_rx_pin);
    cJSON_AddNumberToObject(ard, "tx", data->arduino_tx_pin);
    cJSON_AddNumberToObject(ard, "baud", data->arduino_baud_rate);
    cJSON_AddBoolToObject(ard, "configured", data->arduino_configured);

    cJSON *gps = cJSON_AddObjectToObject(uart, "gps");
    cJSON_AddNumberToObject(gps, "rx", data->gps_rx_pin);
    cJSON_AddNumberToObject(gps, "tx", data->gps_tx_pin);
    cJSON_AddNumberToObject(gps, "baud", data->gps_baud_rate);
    cJSON_AddBoolToObject(gps, "configured", data->gps_configured);

    return root;
}

bool config_save_to_sd(const CarData *data) {
    if (!data) return false;
    mkdir("/sdcard/fiona", 0755);
    cJSON *root = car_data_to_json(data);
    if (!root) return false;
    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;
    FILE *f = fopen(CONFIG_FILE_PATH, "w");
    if (!f) { ESP_LOGE(TAG, "Failed to open config.json for writing"); free(json_str); return false; }
    fputs(json_str, f);
    fclose(f);
    free(json_str);
    ESP_LOGI(TAG, "Configuration saved to SD");
    return true;
}