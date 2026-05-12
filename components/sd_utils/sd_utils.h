#ifndef SD_UTILS_H
#define SD_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include "CarData.h"

#ifdef __cplusplus
extern "C" {
#endif

void sd_write_refuel_record(const RefuelRecord *rec);
bool sd_card_mounted(void);
bool sd_stats_check(void);
bool sd_presets_check(void);

/**
 * @brief Сохраняет суточную статистику в JSON-файл на SD-карте.
 *        Путь: /sdcard/fiona/history/days/YYYY-MM-DD.json
 * @param stats     Указатель на структуру DayStats с данными.
 * @param date_str  Дата в формате "YYYY-MM-DD".
 * @return true если запись успешна и файл прошёл проверку.
 */
bool sd_save_day_stats(const DayStats *stats, const char *date_str);

#ifdef __cplusplus
}
#endif

#endif