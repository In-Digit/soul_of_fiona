/**
 * @file fiona_phrase_loader.h
 * @brief Приватный API загрузчика и хранилища библиотеки фраз Фионы.
 */

#ifndef FIONA_PHRASE_LOADER_H
#define FIONA_PHRASE_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Структуры данных
 * -------------------------------------------------------------------------- */

/** Одна запись библиотеки фраз (событие -> тональность -> массив строк) */
typedef struct {
    char event_id[32];
    char context[16];      /**< "alone", "family", "any" */
    char **neutral;
    int   neutral_count;
    char **funny;
    int   funny_count;
    char **serious;
    int   serious_count;
} phrase_event_t;

/** Тональности */
#define FIONA_TONE_NEUTRAL 0
#define FIONA_TONE_FUNNY   1
#define FIONA_TONE_SERIOUS 2

/** Результат выбора фразы */
typedef struct {
    const char *text;
    uint8_t     tone;       /**< FIONA_TONE_NEUTRAL / FUNNY / SERIOUS */
} FionaPhrase;

/* --------------------------------------------------------------------------
 * API загрузчика
 * -------------------------------------------------------------------------- */

/**
 * @brief Загрузить JSON-библиотеку фраз из /sdcard/fiona/phrases/default_phrases.json.
 *        При отсутствии файла вызывает repair, который создаёт файл из вшитых дефолтов.
 * @return true если библиотека загружена (хотя бы из дефолтов)
 */
bool phrase_loader_init(void);

/**
 * @brief Выбрать случайную фразу для указанного события и тональности.
 * @param event_id Идентификатор события (например, "overheat_alone")
 * @param tone     Тональность (FIONA_TONE_NEUTRAL / FUNNY / SERIOUS)
 * @return Указатель на статическую строку или "", если фраз нет
 */
const char* phrase_loader_pick(const char *event_id, uint8_t tone);

/**
 * @brief Получить количество загруженных событий (для диагностики).
 */
int phrase_loader_event_count(void);

/**
 * @brief Восстановить / обновить JSON-файл библиотеки фраз из вшитых дефолтов.
 *        Используется при отсутствии SD-карты или повреждении файла.
 */
void phrase_loader_repair(void);

#ifdef __cplusplus
}
#endif

#endif /* FIONA_PHRASE_LOADER_H */