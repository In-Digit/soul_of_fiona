/**
 * @file fiona_brain.h
 * @brief Модуль аналитики и прогнозов Фионы (движок правил).
 *
 * Загружает правила из JSON-файла /sdcard/fiona/rules.jsn (или вшитых дефолтов),
 * в каждом цикле анализирует данные из CarData и заполняет глобальную
 * структуру FionaState, которую затем использует душа для выбора реплик.
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
    bool     last_refuel_suspicious; // последняя заправка подозрительная (цена без копеек)
    bool     had_problem;            // в прошлой поездке была проблема (перегрев/разряд)

    /* --- Временные метки --- */
    uint32_t last_shutdown_time;     // время последнего глушения (Unix)
    uint32_t last_problem_time;      // время последней зафиксированной проблемы

    /* --- Текущий триггер и тональность --- */
    char     event_trigger[32];      // event_id для души (напр. "overheat_alone")
    uint8_t  tone;                   // FIONA_TONE_NEUTRAL / FUNNY / SERIOUS

    /* --- Режим движения --- */
    uint8_t  driving_mode;           // 0=CALM, 1=URBAN, 2=TRAFFIC, 3=HIGHWAY
    bool     long_trip;              // true, если заезд дольше 1 часа
    bool     very_long_trip;         // true, если заезд дольше 2 часов

    /* --- Счётчики для внутреннего использования --- */
    uint32_t engine_start_time;      // время запуска двигателя (Unix)
    uint32_t trip_duration_sec;      // текущая длительность заезда в секундах
    int      stop_count_5min;        // количество полных остановок за 5 минут

    /* --- Стиль вождения (используется для окраски арки дросселя) --- */
    uint8_t  driving_style;          // 0=не определён, 1=спокойный, 2=агрессивный, 3=спортивный

    /* --- Калибровка IMU --- */
    uint8_t  calib_status;           // статус калибровки (0x00 – 0x13)

    /* --- Служебные поля --- */
    bool     initialized;
    uint8_t manual_style;   // 0=авто, 1=спорт, 2=семья, 3=агрессивный (ручной выбор)
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