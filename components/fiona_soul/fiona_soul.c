/**
 * @file fiona_soul.c
 * @brief Реализация души Фионы: выбор реплик на основе состояния (FionaState).
 */

#include "fiona_soul.h"
#include "fiona_brain.h"          // чтобы видеть поля FionaState
#include "fiona_phrase_loader.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "FIONA_SOUL";

/* --------------------------------------------------------------------------
 * Инициализация
 * -------------------------------------------------------------------------- */
void fiona_soul_init(void) {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    if (!phrase_loader_init()) {
        ESP_LOGE(TAG, "Failed to load phrase library");
    }
}

/* --------------------------------------------------------------------------
 * Выбор фразы на основе аналитического состояния
 * -------------------------------------------------------------------------- */
FionaPhrase fiona_soul_get_phrase(const struct FionaState *state) {
    FionaPhrase result = { "", FIONA_TONE_NEUTRAL };

    if (!state) {
        return fiona_soul_get_fallback_phrase();
    }

    // Пытаемся взять фразу по явному триггеру, который установил мозг
    if (state->event_trigger[0] != '\0') {
        const char *text = phrase_loader_pick(state->event_trigger, state->tone);
        if (text && text[0] != '\0') {
            result.text = text;
            result.tone = state->tone;
            return result;
        }
    }

    // Если явного триггера нет, возвращаем fallback
    return fiona_soul_get_fallback_phrase();
}

/* --------------------------------------------------------------------------
 * Fallback (роботизированные, безликие фразы)
 * -------------------------------------------------------------------------- */
FionaPhrase fiona_soul_get_fallback_phrase(void) {
    FionaPhrase p;
    p.tone = FIONA_TONE_NEUTRAL;
    static const char *fallbacks[] = {
        "Привет.",
        "Система запущена.",
        "Жду данные...",
        "Я здесь."
    };
    p.text = fallbacks[rand() % (sizeof(fallbacks)/sizeof(fallbacks[0]))];
    return p;
}

/* --------------------------------------------------------------------------
 * Приветствие по времени суток (используется при холодном старте)
 * -------------------------------------------------------------------------- */
FionaPhrase fiona_soul_get_start_phrase_by_time(void) {
    FionaPhrase result = { "", FIONA_TONE_NEUTRAL };
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hour = t->tm_hour;

    const char *event_id = "start_abstract";
    if (hour >= 5 && hour < 7)  event_id = "start_early";
    else if (hour >= 7 && hour < 12) event_id = "start_morning";
    else if (hour >= 12 && hour < 17) event_id = "start_day";
    else if (hour >= 17 && hour < 20) event_id = "start_dusk";
    else if (hour >= 20 || hour < 5)  event_id = "start_night";

    const char *phrase = phrase_loader_pick(event_id, FIONA_TONE_NEUTRAL);
    if (!phrase || phrase[0] == '\0') {
        phrase = phrase_loader_pick(event_id, FIONA_TONE_FUNNY);
    }
    if (!phrase || phrase[0] == '\0') {
        phrase = phrase_loader_pick("start_abstract", FIONA_TONE_NEUTRAL);
    }
    result.text = phrase ? phrase : "Привет. Я готова.";
    return result;
}

/* --------------------------------------------------------------------------
 * Глушение двигателя
 * -------------------------------------------------------------------------- */
FionaPhrase fiona_soul_get_engine_off_phrase(void) {
    FionaPhrase result;
    result.tone = FIONA_TONE_FUNNY;
    const char *text = phrase_loader_pick("engine_off", result.tone);
    if (!text || text[0] == '\0') {
        text = phrase_loader_pick("engine_off", FIONA_TONE_NEUTRAL);
    }
    result.text = text ? text : "";
    return result;
}

/* --------------------------------------------------------------------------
 * Проверка целостности
 * -------------------------------------------------------------------------- */
bool fiona_soul_phrases_check(void) {
    return (phrase_loader_event_count() > 0);
}

void fiona_soul_repair_phrases(void) {
    phrase_loader_repair();
    phrase_loader_init();
}