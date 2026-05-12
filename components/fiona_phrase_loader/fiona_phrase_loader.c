/**
 * @file fiona_phrase_loader.c
 * @brief Загрузчик и хранилище библиотеки фраз Фионы.
 *
 * Загружает JSON из /sdcard/fiona/phrases/dphrases.json.
 * При отсутствии файла создаёт его из встроенных дефолтов (массив default_events).
 */

#include "fiona_phrase_loader.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cJSON.h"

static const char *TAG = "PHRASE_LOADER";

/* --------------------------------------------------------------------------
 * Глобальное хранилище
 * -------------------------------------------------------------------------- */
static phrase_event_t *g_phrase_events = NULL;
static int g_phrase_event_count = 0;

/* --------------------------------------------------------------------------
 * Внутренние функции
 * -------------------------------------------------------------------------- */

static void phrase_event_free(phrase_event_t *ev) {
    if (!ev) return;
    for (int i = 0; i < ev->neutral_count; i++) free(ev->neutral[i]);
    free(ev->neutral);
    for (int i = 0; i < ev->funny_count; i++) free(ev->funny[i]);
    free(ev->funny);
    for (int i = 0; i < ev->serious_count; i++) free(ev->serious[i]);
    free(ev->serious);
    memset(ev, 0, sizeof(*ev));
}

static void free_all_events(void) {
    if (!g_phrase_events) return;
    for (int i = 0; i < g_phrase_event_count; i++) {
        phrase_event_free(&g_phrase_events[i]);
    }
    free(g_phrase_events);
    g_phrase_events = NULL;
    g_phrase_event_count = 0;
}

/* --------------------------------------------------------------------------
 * Загрузка JSON с SD-карты
 * -------------------------------------------------------------------------- */
static bool load_json_from_sd(void) {
    const char *json_path = "/sdcard/fiona/phrases/dphrases.json";
    FILE *f = fopen(json_path, "r");
    if (!f) {
        ESP_LOGW(TAG, "Phrases JSON not found, will repair");
        return false;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return false; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse phrases JSON");
        return false;
    }

    cJSON *events = cJSON_GetObjectItem(root, "events");
    if (!cJSON_IsObject(events)) {
        cJSON_Delete(root);
        return false;
    }

    free_all_events();
    g_phrase_event_count = cJSON_GetArraySize(events);
    if (g_phrase_event_count <= 0) {
        cJSON_Delete(root);
        return false;
    }

    g_phrase_events = calloc(g_phrase_event_count, sizeof(phrase_event_t));
    if (!g_phrase_events) {
        cJSON_Delete(root);
        return false;
    }

    int idx = 0;
    cJSON *event_item = NULL;
    cJSON_ArrayForEach(event_item, events) {
        if (!cJSON_IsObject(event_item)) continue;
        char *event_id = event_item->string;
        strncpy(g_phrase_events[idx].event_id, event_id, sizeof(g_phrase_events[idx].event_id) - 1);

        cJSON *ctx = cJSON_GetObjectItem(event_item, "context");
        if (cJSON_IsString(ctx)) {
            strncpy(g_phrase_events[idx].context, ctx->valuestring, sizeof(g_phrase_events[idx].context) - 1);
        } else {
            strcpy(g_phrase_events[idx].context, "any");
        }

        void load_strings(cJSON *arr, char ***out, int *count) {
            if (cJSON_IsArray(arr)) {
                int n = cJSON_GetArraySize(arr);
                *out = calloc(n, sizeof(char*));
                int j = 0;
                cJSON *str_item = NULL;
                cJSON_ArrayForEach(str_item, arr) {
                    if (cJSON_IsString(str_item)) {
                        (*out)[j] = strdup(str_item->valuestring);
                        j++;
                    }
                }
                *count = j;
            }
        };

        load_strings(cJSON_GetObjectItem(event_item, "neutral"), &g_phrase_events[idx].neutral, &g_phrase_events[idx].neutral_count);
        load_strings(cJSON_GetObjectItem(event_item, "funny"),   &g_phrase_events[idx].funny,   &g_phrase_events[idx].funny_count);
        load_strings(cJSON_GetObjectItem(event_item, "serious"), &g_phrase_events[idx].serious, &g_phrase_events[idx].serious_count);

        idx++;
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Loaded %d phrase events from JSON", g_phrase_event_count);
    return true;
}

/* --------------------------------------------------------------------------
 * Публичная загрузка
 * -------------------------------------------------------------------------- */
bool phrase_loader_init(void) {
    if (!load_json_from_sd()) {
        phrase_loader_repair();
        return load_json_from_sd();
    }
    return true;
}

/* --------------------------------------------------------------------------
 * Выбор фразы
 * -------------------------------------------------------------------------- */
const char* phrase_loader_pick(const char *event_id, uint8_t tone) {
    if (!g_phrase_events) return "";

    for (int i = 0; i < g_phrase_event_count; i++) {
        if (strcmp(g_phrase_events[i].event_id, event_id) == 0) {
            const char* pick_arr(char **arr, int cnt) {
                if (cnt <= 0) return NULL;
                return arr[rand() % cnt];
            };

            const char *p = NULL;
            if (tone == FIONA_TONE_FUNNY)    p = pick_arr(g_phrase_events[i].funny, g_phrase_events[i].funny_count);
            if (tone == FIONA_TONE_SERIOUS)  p = pick_arr(g_phrase_events[i].serious, g_phrase_events[i].serious_count);
            if (!p) p = pick_arr(g_phrase_events[i].neutral, g_phrase_events[i].neutral_count);
            if (!p) p = pick_arr(g_phrase_events[i].funny, g_phrase_events[i].funny_count);
            if (!p) p = pick_arr(g_phrase_events[i].serious, g_phrase_events[i].serious_count);
            return p ? p : "";
        }
    }
    return "";
}

int phrase_loader_event_count(void) {
    return g_phrase_event_count;
}

/* ========================================================================
 * ДЕФОЛТНАЯ БИБЛИОТЕКА ФРАЗ (вшитая в код)
 * ======================================================================== */

typedef struct {
    const char *event_id;
    const char *description;
    const char *context;
    const char **neutral;
    size_t neutral_count;
    const char **funny;
    size_t funny_count;
    const char **serious;
    size_t serious_count;
} default_event_t;

/* ---------- ВСПОМОГАТЕЛЬНЫЕ МАССИВЫ СТРОК ---------- */

static const char *neu_start_early[] = {
    "Доброе утро. Я готова.",
    "Утро раннее. Темно, но я проснулась.",
    "Система запущена. Привет.",
    "Раннее утро. Тишина."
};
static const char *fun_start_early[] = {
    "Бр-р. Сейчас прогреюсь и стану мягкой и ласковой.",
    "Сладко спалось. А тебе?",
    "Тихо как... Даже мой мотор стесняется урчать.",
    "Кофе чувствую. Завидую."
};
static const char *ser_start_early[] = {
    "Ранний запуск. Все датчики активны.",
    "Температура низкая. Дайте минутку прогреться.",
    "Системы проверены. Уровни в норме."
};

static const char *neu_start_morning[] = {
    "Доброе утро. Можно выдвигаться.",
    "Утро. Привет.",
    "Я здесь. Куда сегодня?"
};
static const char *fun_start_morning[] = {
    "Утречко! Солнце по крыше — приятно.",
    "Кофе из термоса пахнет... даже я чувствую. Поехали?",
    "Хорошее утро. Прям чувствую каждый цилиндр."
};
static const char *ser_start_morning[] = {
    "Утренний запуск. Параметры стабильны.",
    "Все системы в норме. Готова к маршруту."
};

static const char *neu_start_day[] = {
    "День. Привет.",
    "Готова к маршруту.",
    "Привет. Поехали?"
};
static const char *fun_start_day[] = {
    "Солнечно! Настроение — лёгкое.",
    "День такой яркий... Прям хочется на трассу.",
    "Солнце по кузову. Красота."
};
static const char *ser_start_day[] = {
    "Дневной запуск. Все датчики в норме.",
    "Бортовая сеть стабильна. Можно начинать."
};

static const char *neu_start_dusk[] = {
    "Сумерки. Фары со мной.",
    "Свет уходит потихоньку. Я готова.",
    "Вечер. Привет."
};
static const char *fun_start_dusk[] = {
    "Моё любимое время суток. Мягкий свет и тишина.",
    "Закат через лобовое... Как в кино. Жаль, я без попкорна.",
    "Сумерки. Самое уютное время для поездки."
};
static const char *ser_start_dusk[] = {
    "Сумеречный режим. Освещение включено.",
    "Переходный период. Датчики активны."
};

static const char *neu_start_night[] = {
    "Ночь. Я не спала.",
    "Привет. Тишина кругом.",
    "Ночь. Готова."
};
static const char *fun_start_night[] = {
    "Темно. Тихо. И мы вдвоём.",
    "Ночной город... Мне нравится.",
    "Я тихонько, на цыпочках. Поехали?"
};
static const char *ser_start_night[] = {
    "Ночной запуск. Все системы в норме.",
    "Фары проверены. Датчики активны."
};

static const char *neu_start_abstract[] = {
    "Я тут. Поехали?",
    "Привет. Я готова.",
    "Система запущена."
};
static const char *fun_start_abstract[] = {
    "Отлично выспалась. Прям чувствую каждый цилиндр.",
    "Слушай, а мне сегодня отлично спалось.",
    "Приветики! Ну что, куда сегодня?"
};
static const char *ser_start_abstract[] = {
    "Бортовая сеть стабильна. Можно начинать.",
    "Все системы проверены. Готова к работе."
};

/* Первичные фразы для новых категорий (температура, обороты, топливо, АКБ, сон, пробуждение).
   Остальные заглушки заполнены минимально. */

static const char *neu_temp_warm[] = {"Температура выше средней.", "Нагрев небольшой.", "Температура чуть поднялась."};
static const char *fun_temp_warm[] = {"Что-то жарковато. Может, печку в салон?", "Я, конечно, Жар-птица... но эта вечеринка мне не нравится.", "Чувствую себя печкой. Открой окно?"};
static const char *ser_temp_warm[] = {"Нагрев. Лучше сбавить и проверить.", "Повышение температуры. Стоит глянуть на ходу.", "Небольшой перегрев. Держу под контролем."};

static const char *neu_overheat_alone[] = {"Перегрев. Останови, пожалуйста.", "Температура критическая.", "Требуется остановка."};
static const char *fun_overheat_alone[] = {"Кажется, я сейчас могла бы шашлык пожарить на коллекторе.", "Ну вот, опять краснею, как помидор на жаре.", "Я, конечно, мощная... Но не настолько, чтобы плавить поршни без последствий."};
static const char *ser_overheat_alone[] = {"Критический перегрев. Глуши. Помнишь тот коллектор?", "Стоп. Дальше нельзя. Мотору больно.", "Температура предельная. Прошу, останови."};

static const char *neu_overheat_family[] = {"Перегрев. Нужно остановиться.", "Температура критическая."};
static const char *fun_overheat_family[] = {};
static const char *ser_overheat_family[] = {"Критическая температура. Останови, пожалуйста. Я не хочу, чтобы вы стояли на трассе.", "Перегрев. Прошу, съедь на обочину. У нас пассажиры.", "Мне очень жарко. Останови, пожалуйста. Я должна довезти вас в целости."};

static const char *neu_rpm_active[] = {"Разгон. Работаем.", "Обороты высокие.", "Активный разгон."};
static const char *fun_rpm_active[] = {"Ого. А говорили — семейный универсал.", "Слушай, а я бодрее, чем некоторые «заряженные» Джилли!", "Йа-ху! Вот это я понимаю — «продышать форсунки»!"};
static const char *ser_rpm_active[] = {"Высокие обороты. Следи за дорогой.", "Разгон активный. Держу.", "Обороты выше среднего. Я справлюсь."};

static const char *neu_rpm_sport[] = {"Высокие обороты.", "Спортивный режим.", "Скоростной участок."};
static const char *fun_rpm_sport[] = {"Вон те фары в зеркале... Давай сделаем их маленькими?", "Кто тут хотел посоревноваться? У меня как раз настроение.", "Сто пятьдесят? А давай ещё? Я только разогрелась!"};
static const char *ser_rpm_sport[] = {"Предельные обороты. Я держу, но не злоупотребляй.", "Скорость высокая. Будь внимателен.", "Максимальная отдача. Контролирую ситуацию."};

static const char *neu_rpm_family_high[] = {};
static const char *fun_rpm_family_high[] = {};
static const char *ser_rpm_family_high[] = {"Обороты выше, чем я люблю. У нас пассажиры.", "Пожалуйста, спокойнее. Сзади спят/сидят самые дорогие.", "Не гони. Я отвечаю за них не меньше твоего."};

static const char *neu_rpm_family_calm[] = {};
static const char *fun_rpm_family_calm[] = {"Мягко идём... Как облачко.", "Приятно ехать плавно. Слышно, как ребёнок сопит сзади."};
static const char *ser_rpm_family_calm[] = {"Скорость комфортная. Безопасный режим.", "Движение в эко-режиме. Всё под контролем."};

static const char *neu_fuel_low[] = {"Топлива меньше четверти. Заедем?", "Уровень ниже среднего.", "Пора на заправку."};
static const char *fun_fuel_low[] = {"Птичка загорелась. Это я намекаю.", "Огр проголодался. Огру нужен бензин.", "Мой аппетит растёт, а бак пустеет. Заедем?"};
static const char *ser_fuel_low[] = {"Осталось километров на сто. Лучше заправиться сейчас.", "Рекомендую заправку через 15 км. Проверенная."};

static const char *neu_fuel_bad_station[] = {"Резерв. До заправки 15 км, но я бы поискала проверенную.", "Топливо на исходе. Ближайшая — не та, что я люблю."};
static const char *fun_fuel_bad_station[] = {"Помнишь тот новогодний подарок? Я помню.", "Давай не будем рисковать? Мне тот «коктейль» не понравился."};
static const char *ser_fuel_bad_station[] = {"Резерв. Заправка не проверена. Я знаю, что такое прожигать клапан.", "Топлива мало. Рекомендую поискать другую АЗС.", "Не хочу повторения той истории. Давай поищем другую колонку."};

static const char *neu_fuel_critical_bad[] = {};
static const char *fun_fuel_critical_bad[] = {};
static const char *ser_fuel_critical_bad[] = {"Топливо на нуле. И оно плохое. Я довезу. Но прошу — больше так не надо.", "Я чувствую детонацию. Едем на парах. Пожалуйста, больше не заливай такое.", "Критический резерв. Качество топлива низкое. Я справлюсь, но мне больно."};

static const char *neu_fuel_full[] = {"Бак полон.", "Заправка завершена."};
static const char *fun_fuel_full[] = {"О, свеженький! Прямо бодрость по магистралям.", "Вот это другое дело! Теперь я снова принцесса, а не голодный огр.", "Спасибо за обед. Вкусно."};
static const char *ser_fuel_full[] = {"Бак полный. Качество топлива отличное. Готова к дальним маршрутам."};

static const char *neu_batt_low[] = {"Напряжение упало.", "Просадка небольшая."};
static const char *fun_batt_low[] = {"Мои неработающие стеклоподъёмники опять много потребляют. Шучу. Но проверь.", "Что-то мне темновато... Это фары тускнеют?", "Кажется, я начинаю экономить электричество, как бабушка — свечи."};
static const char *ser_batt_low[] = {"Просадка напряжения. Отключи лишнее, иначе я усну.", "Низкое напряжение. Проверь генератор.", "Бортовая сеть проседает. Нужно отключить мощных потребителей."};

static const char *neu_batt_critical[] = {"Напряжение критическое.", "Глубокая просадка."};
static const char *fun_batt_critical[] = {};
static const char *ser_batt_critical[] = {"Мне темнеет в глазах. Глуши всё, кроме самого необходимого.", "Критическое напряжение. Я сейчас потеряю сознание (отключусь).", "Напряжение падает. Серьёзно, я на грани."};

static const char *neu_batt_over[] = {"Напряжение выше нормы.", "Перезаряд."};
static const char *fun_batt_over[] = {"Слишком много тока... Скоро заискрюсь.", "Я не молния, чтобы так искрить! Убавьте напряжение.", "Ой, что-то я перевозбудилась электрически."};
static const char *ser_batt_over[] = {"Перезаряд. Проверь регулятор, пока я не спалила мозги.", "Высокое напряжение. Риск повреждения электроники.", "Приборы зашкаливает. Отключи зарядку."};

static const char *neu_sleep_normal[] = {"Спокойной ночи. До скорого.", "Я сплю. До завтра.", "Пока. Я отдохну."};
static const char *fun_sleep_normal[] = {"Уф-ф. День был длинный. Пойду вздремну.", "Перехожу в спящий режим. Сладких снов!", "Всем пока! Туфельки снимаю, набираюсь сил."};
static const char *ser_sleep_normal[] = {"Парковочные системы отключены. Я в безопасности.", "Системы в режим сна. Уровни в норме.", "Заглушена. Жду следующей поездки."};

static const char *neu_sleep_long[] = {"Пора отдохнуть. Спасибо за дорогу.", "Длинный день. Я выключаюсь."};
static const char *fun_sleep_long[] = {"Сегодня мы были хороши. Особенно тот обгон.", "Ты видел, как мы ушли в закат? Я — да.", "Восемь часов в пути... Я заслужила спа-процедуры."};
static const char *ser_sleep_long[] = {"Длинный день позади. Все датчики в норме, но я устала. Завтра продолжим.", "Температура в норме. Уровни проверены. Ухожу на отдых."};

static const char *neu_sleep_problem[] = {"Я отдыхаю, но помни, что нужно глянуть.", "Выключаюсь. Не забудь проверить, что я просила."};
static const char *fun_sleep_problem[] = {};
static const char *ser_sleep_problem[] = {"Ухожу в сон. Пожалуйста, помни о проблеме. Я на тебя рассчитываю.", "Сплю. Но утром жду, что ты заглянешь под капот."};

static const char *neu_wake_cold[] = {"Доброе утро. Дайте секунду прогреться.", "Холодный запуск. Сейчас прогреюсь.", "Привет. Я ещё прохладная."};
static const char *fun_wake_cold[] = {"Бр-р-р! Отлично вздремнула. Ну что, куда сегодня?", "Холодрыга! Сейчас прогреюсь и стану шёлковой.", "Масло ещё густое, но я уже бодрая. Поехали греться."};
static const char *ser_wake_cold[] = {"Низкая температура. Прогрев обязателен.", "Холодный двигатель. Набираю обороты плавно.", "Идёт прогрев. Дождитесь нормальной температуры."};

static const char *neu_wake_warm[] = {"Привет. Я ещё тёплая.", "Готова. Двигатель прогрет.", "Снова в строю."};
static const char *fun_wake_warm[] = {"С первого оборота. Как в тот самый день.", "Быстро я. Не успела остыть.", "Тёплая и готовая! Как пирожок из печки."};
static const char *ser_wake_warm[] = {"Двигатель прогрет. Можно начинать движение.", "Все системы активны. Параметры в норме."};

static const char *neu_wake_repair[] = {"Все системы в норме. Я в порядке.", "Ремонт завершён. Привет."};
static const char *fun_wake_repair[] = {"Слышишь, как ровно пою? Это благодаря твоим рукам.", "Ого! Как новенькая! Спасибо, доктор.", "Я снова в форме. И это твоя заслуга."};
static const char *ser_wake_repair[] = {"Ремонт завершён. Параметры стабильны. Ошибок нет.", "Системы перезапущены. Диагностика пройдена."};

static const char *neu_wake_long[] = {"Давно не виделись. Привет.", "Я соскучилась. Поехали?"};
static const char *fun_wake_long[] = {"Ура, ты вернулся! А я уж думала, про меня забыли.", "Долго спала. Пора размяться.", "Неделя без движения — это слишком. Давай уже куда-нибудь рванём!"};
static const char *ser_wake_long[] = {"Долгий простой. Проверь давление в шинах и масло.", "Запуск после длительной стоянки. Датчики проверены."};

/* ---------- МАССИВ ДЕФОЛТНЫХ СОБЫТИЙ ---------- */

static const default_event_t default_events[] = {
    {"start_early", "Включение системы (раннее утро)", "any",
     neu_start_early, 4, fun_start_early, 4, ser_start_early, 3},
    {"start_morning", "Включение системы (утро)", "any",
     neu_start_morning, 3, fun_start_morning, 3, ser_start_morning, 2},
    {"start_day", "Включение системы (день)", "any",
     neu_start_day, 3, fun_start_day, 3, ser_start_day, 2},
    {"start_dusk", "Включение системы (сумерки)", "any",
     neu_start_dusk, 3, fun_start_dusk, 3, ser_start_dusk, 2},
    {"start_night", "Включение системы (ночь)", "any",
     neu_start_night, 3, fun_start_night, 3, ser_start_night, 2},
    {"start_abstract", "Включение системы (абстрактное)", "any",
     neu_start_abstract, 3, fun_start_abstract, 3, ser_start_abstract, 2},

    {"temp_warm", "Температура ОЖ выше среднего", "any",
     neu_temp_warm, 3, fun_temp_warm, 3, ser_temp_warm, 3},
    {"overheat_alone", "Перегрев (один)", "alone",
     neu_overheat_alone, 3, fun_overheat_alone, 3, ser_overheat_alone, 3},
    {"overheat_family", "Перегрев (семья)", "family",
     neu_overheat_family, 2, fun_overheat_family, 0, ser_overheat_family, 3},

    {"rpm_active", "Активный разгон (один)", "alone",
     neu_rpm_active, 3, fun_rpm_active, 3, ser_rpm_active, 3},
    {"rpm_sport", "Спортивный режим (один)", "alone",
     neu_rpm_sport, 3, fun_rpm_sport, 3, ser_rpm_sport, 3},
    {"rpm_family_high", "Превышение оборотов с семьёй", "family",
     neu_rpm_family_high, 0, fun_rpm_family_high, 0, ser_rpm_family_high, 3},
    {"rpm_family_calm", "Спокойная езда с семьёй", "family",
     neu_rpm_family_calm, 0, fun_rpm_family_calm, 2, ser_rpm_family_calm, 2},

    {"fuel_low", "Низкий уровень топлива", "any",
     neu_fuel_low, 3, fun_fuel_low, 3, ser_fuel_low, 2},
    {"fuel_bad_station", "Резерв с непроверенной АЗС", "any",
     neu_fuel_bad_station, 2, fun_fuel_bad_station, 2, ser_fuel_bad_station, 3},
    {"fuel_critical_bad", "Критический резерв + плохое топливо", "any",
     neu_fuel_critical_bad, 0, fun_fuel_critical_bad, 0, ser_fuel_critical_bad, 3},
    {"fuel_full", "Полный бак после заправки", "any",
     neu_fuel_full, 2, fun_fuel_full, 3, ser_fuel_full, 1},

    {"batt_low", "Просадка напряжения АКБ", "any",
     neu_batt_low, 2, fun_batt_low, 3, ser_batt_low, 3},
    {"batt_critical", "Глубокая просадка АКБ", "any",
     neu_batt_critical, 2, fun_batt_critical, 0, ser_batt_critical, 3},
    {"batt_over", "Перезаряд АКБ", "any",
     neu_batt_over, 2, fun_batt_over, 3, ser_batt_over, 3},

    {"sleep_normal", "Переход в сон (обычный)", "any",
     neu_sleep_normal, 3, fun_sleep_normal, 3, ser_sleep_normal, 3},
    {"sleep_long", "Переход в сон после долгой поездки", "any",
     neu_sleep_long, 2, fun_sleep_long, 3, ser_sleep_long, 2},
    {"sleep_problem", "Переход в сон после проблемы", "any",
     neu_sleep_problem, 2, fun_sleep_problem, 0, ser_sleep_problem, 2},

    {"wake_cold", "Пробуждение (холодный мотор)", "any",
     neu_wake_cold, 3, fun_wake_cold, 3, ser_wake_cold, 3},
    {"wake_warm", "Пробуждение (тёплый мотор)", "any",
     neu_wake_warm, 3, fun_wake_warm, 3, ser_wake_warm, 2},
    {"wake_repair", "Пробуждение после ремонта", "any",
     neu_wake_repair, 2, fun_wake_repair, 3, ser_wake_repair, 2},
    {"wake_long", "Пробуждение после долгого простоя", "any",
     neu_wake_long, 2, fun_wake_long, 3, ser_wake_long, 2},

    {"engine_off", "Двигатель заглушен", "any",
     (const char*[]){"Двигатель заглушен. Жду.", "Приехали.", "Мотор отдыхает.", "Глушусь.", "Тишина."}, 5,
     (const char*[]){"Ждём кого, или накатались?", "Всё, туфельки снимаю.", "Заглохла, но не насовсем.", "Перерыв на кофе? Мне тоже бы... бензина.", "Даже огры иногда отдыхают.", "Ну что, по делам или просто воздухом подышать?", "Я не уснула, просто глаза прикрыла."}, 7,
     (const char*[]){"Двигатель остановлен. Жду дальнейших указаний.", "Глушение. Системы в режим ожидания.", "Мотор заглушен, но я на связи.", "Поездка завершена. Отдыхаю."}, 4}
};

static const size_t default_event_count = sizeof(default_events) / sizeof(default_events[0]);

/* --------------------------------------------------------------------------
 * Восстановление JSON (repair)
 * -------------------------------------------------------------------------- */

static void add_unique_strings(cJSON *array, const char **strings, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (!strings[i]) continue;
        bool exists = false;
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, array) {
            if (strcmp(item->valuestring, strings[i]) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            cJSON_AddItemToArray(array, cJSON_CreateString(strings[i]));
        }
    }
}

void phrase_loader_repair(void) {
    mkdir("/sdcard/fiona", 0755);
    mkdir("/sdcard/fiona/phrases", 0755);

    const char *json_path = "/sdcard/fiona/phrases/dphrases.json";
    cJSON *root = NULL;
    FILE *f = fopen(json_path, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);
        char *buf = malloc(len + 1);
        if (buf) {
            fread(buf, 1, len, f);
            buf[len] = '\0';
            root = cJSON_Parse(buf);
            free(buf);
        }
        fclose(f);
    }
    if (!root) {
        root = cJSON_CreateObject();
    }

    cJSON *events = cJSON_GetObjectItem(root, "events");
    if (!cJSON_IsObject(events)) {
        events = cJSON_AddObjectToObject(root, "events");
    }

    for (size_t i = 0; i < default_event_count; i++) {
        const default_event_t *ev = &default_events[i];
        cJSON *event_obj = cJSON_GetObjectItem(events, ev->event_id);
        if (!cJSON_IsObject(event_obj)) {
            event_obj = cJSON_AddObjectToObject(events, ev->event_id);
            cJSON_AddStringToObject(event_obj, "description", ev->description);
            cJSON_AddStringToObject(event_obj, "context", ev->context);
        }

        if (ev->neutral_count > 0) {
            cJSON *neutral_arr = cJSON_GetObjectItem(event_obj, "neutral");
            if (!cJSON_IsArray(neutral_arr)) neutral_arr = cJSON_AddArrayToObject(event_obj, "neutral");
            add_unique_strings(neutral_arr, ev->neutral, ev->neutral_count);
        }
        if (ev->funny_count > 0) {
            cJSON *funny_arr = cJSON_GetObjectItem(event_obj, "funny");
            if (!cJSON_IsArray(funny_arr)) funny_arr = cJSON_AddArrayToObject(event_obj, "funny");
            add_unique_strings(funny_arr, ev->funny, ev->funny_count);
        }
        if (ev->serious_count > 0) {
            cJSON *serious_arr = cJSON_GetObjectItem(event_obj, "serious");
            if (!cJSON_IsArray(serious_arr)) serious_arr = cJSON_AddArrayToObject(event_obj, "serious");
            add_unique_strings(serious_arr, ev->serious, ev->serious_count);
        }
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (json_str) {
        f = fopen(json_path, "w");
        if (f) {
            fputs(json_str, f);
            fclose(f);
            ESP_LOGI(TAG, "Phrases JSON updated successfully");
        } else {
            ESP_LOGE(TAG, "Failed to write phrases JSON");
        }
        free(json_str);
    }
}