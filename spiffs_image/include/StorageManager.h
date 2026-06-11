#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "CarData.h"

/**
 * @brief Менеджер сохранения/загрузки конфигурации и состояния в LittleFS.
 * 
 * Конфигурация хранится в /config.json, состояние — в /state.json.
 * Запись состояния атомарна (через временный файл).
 */
class StorageManager {
public:
    StorageManager(CarData& data);
    bool begin();                     // монтирует LittleFS
    bool loadConfig();                 // загружает /config.json в CarData
    bool saveConfig();                 // сохраняет CarData в /config.json
    bool loadState();                  // загружает /state.json в CarData (поля состояния)
    bool saveState();                  // сохраняет поля состояния в /state.json

    void requestConfigSave();          // вызвать при изменении настроек
    void requestStateSave();           // вызвать при изменении состояния (fuelValueML, tripState и т.п.)
    void process();                    // вызывать в цикле (например, раз в 100 мс) для обработки запросов

    uint32_t getLastConfigSaveTime() const { return _lastConfigSaveTime; }
    uint32_t getLastStateSaveTime() const { return _lastStateSaveTime; }
    bool getLastConfigSaveError() const { return _lastConfigSaveError; }
    bool getLastStateSaveError() const { return _lastStateSaveError; }
    bool isConfigSaveRequested() const { return _configSaveRequested; }
    bool isStateSaveRequested() const { return _stateSaveRequested; }

private:
    CarData& _carData;
    bool _configSaveRequested;
    bool _stateSaveRequested;
    uint32_t _lastStateSave;
    static const uint32_t STATE_SAVE_DEBOUNCE = 2000; // минимальный интервал между сохранениями состояния, мс

    uint32_t _lastConfigSaveTime;
    uint32_t _lastStateSaveTime;
    bool _lastConfigSaveError;
    bool _lastStateSaveError;

    void serializeConfig(JsonDocument& doc);
    void deserializeConfig(JsonDocument& doc);
    void serializeState(JsonDocument& doc);
    void deserializeState(JsonDocument& doc);
};

#endif