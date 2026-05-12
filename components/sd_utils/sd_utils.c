#include "sd_utils.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static const char *TAG = "SD_UTILS";

void sd_write_refuel_record(const RefuelRecord *rec) {
    time_t t = rec->timestamp;
    struct tm *tm = localtime(&t);
    char path[128];
    snprintf(path, sizeof(path), "/sdcard/stats/refuels/%04d/%02d", tm->tm_year+1900, tm->tm_mon+1);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "/sdcard/stats/refuels/%04d/%02d/%02d_%02d%02d%02d.json",
             tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"type\": \"refuel\",\n");
        fprintf(f, "  \"timestamp\": %lu,\n", rec->timestamp);
        fprintf(f, "  \"liters\": %.2f,\n", rec->liters);
        fprintf(f, "  \"price\": %.2f,\n", rec->price);
        fprintf(f, "  \"odo_km\": %lu,\n", rec->odo_km);
        fprintf(f, "  \"cost\": %.2f,\n", rec->cost);
        fprintf(f, "  \"calibration\": %s\n", rec->calibration ? "true" : "false");
        fprintf(f, "}\n");
        fclose(f);
        ESP_LOGI(TAG, "Saved refuel record to %s", path);
    } else {
        ESP_LOGE(TAG, "Failed to write refuel record");
    }
}

bool sd_card_mounted(void) {
    struct stat st;
    if (stat("/sdcard", &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}

bool sd_stats_check(void) {
    struct stat st;
    if (stat("/sdcard/stats", &st) == 0 && S_ISDIR(st.st_mode)) {
        FILE *f = fopen("/sdcard/stats/daily_default.json", "r");
        if (!f) {
            f = fopen("/sdcard/stats/daily_default.json", "w");
            if (f) {
                fprintf(f, "{\"valid\": false}\n");
                fclose(f);
            }
        } else {
            fclose(f);
        }
        return true;
    } else {
        mkdir("/sdcard/stats", 0755);
        FILE *f = fopen("/sdcard/stats/daily_default.json", "w");
        if (f) {
            fprintf(f, "{\"valid\": false}\n");
            fclose(f);
        }
        return false;
    }
}

bool sd_presets_check(void) {
    FILE *f = fopen("/sdcard/fiona/presets.json", "r");
    if (!f) {
        mkdir("/sdcard/fiona", 0755);
        f = fopen("/sdcard/fiona/presets.json", "w");
        if (f) {
            fprintf(f, "{\"presets\": []}\n");
            fclose(f);
        }
        return false;
    }
    fclose(f);
    return true;
}

bool sd_save_day_stats(const DayStats *stats, const char *date_str) {
    if (!stats || !date_str) return false;

    mkdir("/sdcard/fiona/history/days", 0755);

    char path[128];
    snprintf(path, sizeof(path), "/sdcard/fiona/history/days/%s.json", date_str);

    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return false;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"date\": %lu,\n", stats->date);
    fprintf(f, "  \"valid_flag\": %d,\n", stats->valid_flag);
    fprintf(f, "  \"engine_sec\": %lu,\n", stats->engine_sec);
    fprintf(f, "  \"distance_m\": %lu,\n", stats->distance_m);
    fprintf(f, "  \"fuel_x100\": %d,\n", stats->fuel_x100);
    fprintf(f, "  \"max_speed\": %d,\n", stats->max_speed);
    fprintf(f, "  \"max_lph_x100\": %d,\n", stats->max_lph_x100);
    fprintf(f, "  \"first_start_time\": %lu,\n", stats->first_start_time);
    fprintf(f, "  \"last_stop_time\": %lu,\n", stats->last_stop_time);
    fprintf(f, "  \"trip_count\": %d,\n", stats->trip_count);
    fprintf(f, "  \"drive_count\": %d,\n", stats->drive_count);
    fprintf(f, "  \"avg_throttle_rel_x100\": %d,\n", stats->avg_throttle_rel_x100);
    fprintf(f, "  \"max_throttle_rel\": %d,\n", stats->max_throttle_rel);
    fprintf(f, "  \"warmup_sec\": %lu,\n", stats->warmup_sec);
    fprintf(f, "  \"aggressive_count\": %d,\n", stats->aggressive_count);
    fprintf(f, "  \"full_throttle_count\": %d,\n", stats->full_throttle_count);
    fprintf(f, "  \"moving_time_sec\": %lu,\n", stats->moving_time_sec);
    fprintf(f, "  \"valid\": %s\n", stats->valid ? "true" : "false");
    fprintf(f, "}\n");

    fclose(f);

    // Проверка записи: открыть и убедиться, что размер ненулевой
    FILE *check = fopen(path, "r");
    if (!check) {
        ESP_LOGE(TAG, "Verification failed: cannot reopen %s", path);
        return false;
    }
    fseek(check, 0, SEEK_END);
    long size = ftell(check);
    fclose(check);

    if (size > 10) {
        ESP_LOGI(TAG, "Day stats saved and verified: %s", path);
        return true;
    } else {
        ESP_LOGE(TAG, "Day stats file too small: %s", path);
        return false;
    }
}