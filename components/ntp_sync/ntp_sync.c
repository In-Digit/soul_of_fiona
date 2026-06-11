/**
 * @file ntp_sync.c
 * @brief Реализация однократной синхронизации времени по NTP (с защитой от повторов).
 */

#include "ntp_sync.h"
#include "rx8025_rtc.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "NTP_SYNC";
static bool sntp_initialized = false;

static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized via NTP");
    time_t now = tv->tv_sec;
    struct tm *lt = localtime(&now);
    if (lt) {
        rtc_time_t rtc_time;
        rtc_time.year = lt->tm_year + 1900;
        rtc_time.mon  = lt->tm_mon + 1;
        rtc_time.day  = lt->tm_mday;
        rtc_time.hour = lt->tm_hour;
        rtc_time.min  = lt->tm_min;
        rtc_time.sec  = lt->tm_sec;
        rtc_set_time(&rtc_time);
    }
}

esp_err_t ntp_sync_request(void) {
    if (sntp_initialized) {
        ESP_LOGI(TAG, "NTP already initialized, skipping");
        return ESP_OK;
    }
    sntp_initialized = true;

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < 100) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        ESP_LOGI(TAG, "NTP sync completed");
        return ESP_OK;
    }

    ESP_LOGW(TAG, "NTP sync failed or timed out");
    return ESP_FAIL;
}