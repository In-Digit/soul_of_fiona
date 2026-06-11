/**
 * @file ntp_sync.h
 * @brief Простой NTP-клиент для разовой синхронизации времени.
 *
 * Использует стандартный SNTP из ESP-IDF. После синхронизации
 * обновляет системное время и записывает его во внешний RTC.
 */

#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ntp_sync_request(void);

#ifdef __cplusplus
}
#endif

#endif