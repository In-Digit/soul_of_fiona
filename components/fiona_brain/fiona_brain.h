/**
 * @file fiona_brain.h
 * @brief Модуль аналитики и прогнозов Фионы (движок правил).
 */

#ifndef FIONA_BRAIN_H
#define FIONA_BRAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Структура аналитического состояния (FionaState)
 * -------------------------------------------------------------------------- */

typedef struct FionaState {
    /* --- Тепловое состояние --- */
    uint8_t  thermal_state;          // 0=холодный, 1=норма, 2=тёплый, 3=перегрев

    /* --- Топливное состояние --- */
    uint8_t  fuel_state;             // 0=полный, 1=низкий, 2=критический

    /* --- Состояние АКБ --- */
    uint8_t  batt_state;             // 0=норма, 1=просадка, 2=глубокая просадка, 3=перезаряд

    /* --- Контекст водителя --- */
    bool     is_alone;               // true – в салоне только водитель

    /* --- Признаки проблем --- */
    bool     last_refuel_suspicious; // последняя заправка подозрительная
    bool     had_problem;            // в прошлой поездке была проблема

    /* --- Временные метки --- */
    uint32_t last_shutdown_time;
    uint32_t last_problem_time;

    /* --- Текущий триггер и тональность --- */
    char     event_trigger[32];      // event_id для души
    uint8_t  tone;                   // FIONA_TONE_NEUTRAL / FUNNY / SERIOUS

    /* --- Режим движения --- */
    uint8_t  driving_mode;           // 0=CALM, 1=URBAN, 2=TRAFFIC, 3=HIGHWAY
    bool     long_trip;              // true, если заезд дольше 1 часа
    bool     very_long_trip;         // true, если заезд дольше 2 часов

    /* --- Счётчики для внутреннего использования --- */
    uint32_t engine_start_time;      // время запуска двигателя (Unix)
    uint32_t trip_duration_sec;      // текущая длительность заезда в секундах
    int      stop_count_5min;        // количество полных остановок за 5 минут

    /* --- Служебные поля --- */
    bool     initialized;
} FionaState;

/* --------------------------------------------------------------------------
 * Публичный API
 * -------------------------------------------------------------------------- */

void fiona_brain_init(void);
void fiona_brain_update(void);
FionaState* fiona_brain_get_state(void);
void fiona_brain_save_state(void);
void fiona_brain_load_state(void);
void fiona_brain_reload_rules(void);

#ifdef __cplusplus
}
#endif

#endif /* FIONA_BRAIN_H */