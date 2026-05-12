#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "CarData.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Загружает параметры из /sdcard/fiona/config.json в CarData.
 *        Существующие значения перезаписываются, отсутствующие ключи игнорируются.
 * @param data Указатель на CarData для заполнения.
 * @return true если файл найден и успешно разобран, false иначе.
 */
bool config_load_from_sd(CarData *data);

/**
 * @brief Сохраняет все параметры CarData в /sdcard/fiona/config.json.
 * @param data Константный указатель на данные.
 * @return true при успешной записи.
 */
bool config_save_to_sd(const CarData *data);

#ifdef __cplusplus
}
#endif

#endif