/**
 * @file fiona_soul.h
 * @brief Публичный API души Фионы.
 */

#ifndef FIONA_SOUL_H
#define FIONA_SOUL_H

#include <stdbool.h>
#include <stdint.h>
#include "fiona_phrase_loader.h"   // FionaPhrase, FIONA_TONE_*

#ifdef __cplusplus
extern "C" {
#endif

// Предварительное объявление структуры состояния (определена в fiona_brain.h)
struct FionaState;

/**
 * @brief Инициализировать душу (загрузить библиотеку фраз).
 *        Вызывается один раз при старте системы.
 */
void fiona_soul_init(void);

/**
 * @brief Получить реплику на основе текущего аналитического состояния.
 * @param state Указатель на структуру FionaState (может быть NULL – тогда fallback).
 * @return Структура FionaPhrase с текстом и тональностью.
 */
FionaPhrase fiona_soul_get_phrase(const struct FionaState *state);

/**
 * @brief Получить безликую fallback-фразу (когда данных нет).
 * @return Строка-заглушка (тональность всегда neutral).
 */
FionaPhrase fiona_soul_get_fallback_phrase(void);

/**
 * @brief Получить фразу-приветствие, соответствующую времени суток.
 *        Используется при холодном старте.
 * @return Структура FionaPhrase.
 */
FionaPhrase fiona_soul_get_start_phrase_by_time(void);

/**
 * @brief Получить фразу при глушении двигателя.
 * @return Структура FionaPhrase.
 */
FionaPhrase fiona_soul_get_engine_off_phrase(void);

/**
 * @brief Проверить целостность библиотеки фраз (для отладочных индикаторов).
 */
bool fiona_soul_phrases_check(void);

/**
 * @brief Принудительно восстановить / обновить JSON-файл библиотеки.
 */
void fiona_soul_repair_phrases(void);

#ifdef __cplusplus
}
#endif

#endif /* FIONA_SOUL_H */