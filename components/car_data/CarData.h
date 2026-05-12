#ifndef CAR_DATA_H
#define CAR_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *                              ЗНАЧЕНИЯ ПО УМОЛЧАНИЮ
 * ========================================================================== */

#define COLOR_CYAN    0x00f3ff
#define COLOR_ORANGE  0xff7800
#define COLOR_RED     0xff3333
#define COLOR_GREEN   0x00ff00
#define COLOR_BLUE    0x0000ff
#define COLOR_YELLOW  0xffff00

#define FUEL_RED_THRESHOLD       5
#define FUEL_YELLOW_THRESHOLD   10
#define BATT_LOW_THRESHOLD      10.5f
#define BATT_HIGH_THRESHOLD     14.7f
#define TEMP_COLD               60
#define TEMP_NORMAL             95
#define TEMP_WARM               98
#define TEMP_HOT               104
#define SPEED_LOW               20
#define SPEED_MID_LOW           35
#define SPEED_MID_HIGH          40
#define SPEED_HIGH_LOW          55
#define SPEED_HIGH_HIGH         60
#define SPEED_VHIGH_LOW        110
#define SPEED_VHIGH_HIGH       130
#define RPM_BLUE_MAX           650
#define RPM_CYAN_MAX          2700
#define RPM_YELLOW_MAX        3300
#define LPH_BLUE_MAX             5
#define LPH_CYAN_MAX            10
#define LPH_YELLOW_MAX          20

#define FUEL_MIN     0
#define FUEL_MAX    50
#define FUEL_STEP    1
#define RANGE_MIN    0
#define RANGE_MAX  500
#define RANGE_STEP   1
#define BATT_MIN     7.0f
#define BATT_MAX    16.5f
#define BATT_STEP    0.1f
#define TEMP_MIN   -30
#define TEMP_MAX   130
#define TEMP_STEP    1
#define SPEED_MIN    0
#define SPEED_MAX  220
#define SPEED_STEP   1
#define RPM_MIN      0
#define RPM_MAX   7000
#define RPM_STEP     1
#define TRIPM_MIN    0
#define TRIPM_MAX 3000
#define TRIPM_STEP   1
#define LPH_MIN      0
#define LPH_MAX     30
#define LPH_STEP     0.1f

/* ============================================================================
 *                          СТРУКТУРЫ СТАТИСТИКИ
 * ========================================================================== */

/**
 * @brief Статистика одной поездки (рейса).
 */
typedef struct {
    uint32_t start_time;          ///< Unix-время начала поездки
    uint8_t  status;              ///< 0=авто, 1=ручная
    uint32_t duration_sec;        ///< Длительность поездки, сек
    uint32_t pause_sec;           ///< Суммарное время стоянок, сек
    uint16_t pause_count;         ///< Количество стоянок
    uint32_t distance_m;          ///< Пройденное расстояние, метры
    uint16_t fuel_x100;           ///< Потрачено топлива, литры ×100
    uint16_t max_speed;           ///< Максимальная скорость, км/ч
    uint16_t max_lph_x100;        ///< Максимальный мгновенный расход, л/ч ×100
    uint32_t warmup_sec;          ///< Время прогрева, сек
    uint16_t avg_throttle_rel_x100; ///< Средний относительный дроссель, ×100
    uint16_t max_throttle_rel;    ///< Максимальный относительный дроссель
    uint16_t aggressive_count;    ///< Количество агрессивных нажатий
    uint16_t full_throttle_count; ///< Количество полных открытий дросселя
    uint32_t moving_time_sec;     ///< Чистое время движения, сек
    bool     valid;               ///< Флаг достоверности данных
} TripStats;

/**
 * @brief Суточная статистика.
 */
typedef struct {
    uint32_t date;                ///< Дата (Unix-начало суток)
    uint8_t  valid_flag;          ///< Флаг достоверности (0/1)
    uint32_t engine_sec;          ///< Время работы двигателя, сек
    uint32_t distance_m;          ///< Пройденное расстояние, метры
    uint16_t fuel_x100;           ///< Потрачено топлива, литры ×100
    uint16_t max_speed;           ///< Максимальная скорость, км/ч
    uint16_t max_lph_x100;        ///< Максимальный мгновенный расход, л/ч ×100
    uint32_t first_start_time;    ///< Время первого запуска двигателя (Unix)
    uint32_t last_stop_time;      ///< Время последнего глушения (Unix)
    uint8_t  trip_count;          ///< Количество рейсов за сутки
    uint8_t  drive_count;         ///< Количество заездов за сутки
    uint16_t avg_throttle_rel_x100; ///< Средний относительный дроссель, ×100
    uint16_t max_throttle_rel;    ///< Максимальный относительный дроссель
    uint32_t warmup_sec;          ///< Время прогрева, сек
    uint16_t aggressive_count;    ///< Суммарное количество агрессивных нажатий
    uint16_t full_throttle_count; ///< Суммарное количество полных открытий дросселя
    uint32_t moving_time_sec;     ///< Чистое время движения, сек
    bool     valid;               ///< Флаг достоверности данных
} DayStats;

/**
 * @brief Запись о заправке.
 */
typedef struct {
    uint32_t timestamp;
    float    liters;
    float    price;
    uint32_t odo_km;
    float    cost;
    bool     calibration;
} RefuelRecord;

/**
 * @brief Один заезд (Drive Cycle) — непрерывный период работы двигателя с движением.
 */
typedef struct {
    uint32_t start_time;          ///< Unix-время начала заезда
    uint32_t end_time;            ///< Unix-время окончания заезда
    uint32_t duration_sec;        ///< Чистое время движения, сек
    uint32_t distance_m;          ///< Пройденное расстояние, метры
    uint16_t fuel_x100;           ///< Потрачено топлива, литры ×100
    uint16_t max_speed;           ///< Максимальная скорость, км/ч
    uint16_t max_lph_x100;        ///< Максимальный мгновенный расход, л/ч ×100
    uint16_t avg_throttle_rel_x100; ///< Средний относительный дроссель, ×100
    uint16_t max_throttle_rel;    ///< Максимальный относительный дроссель
    uint16_t aggressive_count;    ///< Количество агрессивных нажатий
    uint16_t full_throttle_count; ///< Количество полных открытий дросселя
    uint32_t warmup_sec;          ///< Время прогрева, сек
    bool     valid;               ///< Флаг достоверности данных
} DriveCycle;

/* ============================================================================
 *                          СТРУКТУРА ДАННЫХ АВТОМОБИЛЯ И СИСТЕМЫ
 * ========================================================================== */

typedef struct {
    /* ----------------------- Настраиваемые цвета -------------------------- */
    uint32_t colorCyan;
    uint32_t colorOrange;
    uint32_t colorRed;
    uint32_t colorGreen;
    uint32_t colorBlue;
    uint32_t colorYellow;

    /* ---------------------- Пороги смены цветов -------------------------- */
    uint8_t  fuelRedThreshold;
    uint8_t  fuelYellowThreshold;
    float    battLowThreshold;
    float    battHighThreshold;
    int8_t   tempCold;
    int8_t   tempNormal;
    int8_t   tempWarm;
    int8_t   tempHot;
    uint8_t  speedLow;
    uint8_t  speedMidLow;
    uint8_t  speedMidHigh;
    uint8_t  speedHighLow;
    uint8_t  speedHighHigh;
    uint8_t  speedVHighLow;
    uint8_t  speedVHighHigh;
    uint16_t rpmBlueMax;
    uint16_t rpmCyanMax;
    uint16_t rpmYellowMax;
    uint8_t  lphBlueMax;
    uint8_t  lphCyanMax;
    uint8_t  lphYellowMax;

    /* ----------------------- Диапазоны шкал ------------------------------ */
    int   fuelMin;
    int   fuelMax;
    int   fuelStep;
    int   rangeMin;
    int   rangeMax;
    int   rangeStep;
    float battMin;
    float battMax;
    float battStep;
    int   tempMin;
    int   tempMax;
    int   tempStep;
    int   speedMin;
    int   speedMax;
    int   speedStep;
    int   rpmMin;
    int   rpmMax;
    int   rpmStep;
    int   tripMMin;
    int   tripMMax;
    int   tripMStep;
    int   lphMin;
    int   lphMax;
    float lphStep;

    /* ================ КАТЕГОРИЯ 1: ДАННЫЕ ОТ ELM327 (OBD) ================ */
    float    mafValue;
    int      speedValue;
    int      rpmValue;
    int      tempValue;
    float    batValue;
    float    intakeTemp;
    float    loadValue;
    float    throttlePos;
    float    stft;
    float    ltft;
    float    fuelPressure;

    bool mafDirty;
    bool speedDirty;
    bool rpmDirty;
    bool tempDirty;
    bool batDirty;
    bool intakeTempDirty;
    bool loadDirty;
    bool throttleDirty;
    bool stftDirty;
    bool ltftDirty;
    bool fuelPressureDirty;

    /* ================ КАТЕГОРИЯ 2: ВЫЧИСЛЯЕМЫЕ ШЛЮЗОМ ЗНАЧЕНИЯ ================ */
    float    lphValue;
    float    fuelValue;
    int      rangeValue;
    uint32_t odoKm;
    uint32_t tripValue;
    uint32_t tripPauseValue;
    float    tripDistanceKm;
    float    tripMValue;
    bool     tripState;
    float    tripFuelUsed;

    bool lphDirty;
    bool fuelDirty;
    bool rangeDirty;
    bool odoDirty;
    bool tripTimeDirty;
    bool tripPauseDirty;
    bool tripDistDirty;
    bool tripCostDirty;
    bool tripStateDirty;
    bool tripFuelDirty;

    /* ------------------ Калибровка топлива ------------------ */
    float    fuelPrice;
    float    fuelCalibrationFactor;
    uint32_t lastFullOdoKm;
    float    totalRefuelSinceLastFull;
    float    calculatedFuelSinceLastFull;
    bool     hasFirstFullTank;
    uint32_t initialOdoKm;
    float    initialFuel;
    bool     calibrationNeeded;

    /* ------------------ Параметры логики поездок ------------------------- */
    uint16_t tripAutoStopTimeout;
    uint32_t lastManualStopTime;
    uint16_t sleepTimeout;

    /* ================ КАТЕГОРИЯ 3: ДАННЫЕ ОТ ARDUINO ====== */
    float    ntcHeaterOut;
    float    ntcCabin;
    float    ntcDriverFeet;
    float    ntcPassengerFeet;
    float    ntcTrunk;
    float    ntcCabinCenter;

    bool ntcHeaterOutDirty;
    bool ntcCabinDirty;
    bool ntcDriverFeetDirty;
    bool ntcPassengerFeetDirty;
    bool ntcTrunkDirty;
    bool ntcCabinCenterDirty;

    uint8_t fanCurrentPWM1;
    uint8_t fanCurrentPWM2;
    uint8_t climateCurrentPWM;
    uint8_t trunkFanPWM;
    uint8_t damperPosition;
    uint8_t airDirection;

    bool fan1Dirty;
    bool fan2Dirty;
    bool heaterDirty;
    bool trunkFanDirty;
    bool damperDirty;
    bool airDirDirty;

    bool    fanAutoMode;
    bool    climateAutoMode;
    bool    damperAutoMode;
    uint8_t fanManualPWM;
    uint8_t climateManualPWM;

    bool fanModeDirty;
    bool climateModeDirty;
    bool damperModeDirty;

    /* ================ КАТЕГОРИЯ 4: НАСТРОЙКИ ИНТЕРФЕЙСА И СЕТЕЙ ================ */
    char wifiSsid1[33];
    char wifiPass1[65];
    char wifiSsid2[33];
    char wifiPass2[65];
    char wifiSsid3[33];
    char wifiPass3[65];
    char apSsid[33];
    char apPass[65];

    char btName[33];
    char btMac[19];
    char btPin[7];

    float   fanSetpoint;
    float   fanKp;
    float   fanKi;
    float   fanKd;

    float   climateSetpoint;

    /* ================ СТАТУСЫ СВЯЗИ ================ */
    bool obdConnected;
    bool wifiConnected;
    int8_t wifiRSSI;
    bool wifiRSSIDirty;
    bool uartArduinoAlive;
    bool uartEsp32Alive;

    bool internetAvailable;
    bool internetDirty;

    /* ================ УПРАВЛЕНИЕ API ================ */
    bool apiRequestPending;
    uint8_t apiRequestId;
    bool apiResponseValid;

    /* ================ РАБОТА С ПРЕСЕТАМИ ================ */
    uint16_t activePresetId;
    bool presetPending;

    /* ================ ВРЕМЕННЫЕ МЕТКИ ================ */
    uint32_t systemTime;
    uint32_t lastObdUpdate;
    uint32_t lastPresetUpdate;
    uint32_t lastApiResponse;

    /* ================ СЛУЖЕБНЫЕ ФЛАГИ СИНХРОНИЗАЦИИ ================ */
    bool configDirty;
    bool requestConfigSync;

    /* ================ СКРИНСЕЙВЕР ================ */
    uint16_t screensaver_timeout_sec;

    /* ================ СТАТИСТИКА ================ */
    TripStats lastTripStats;
    bool trip_stats_pending;

    DayStats lastDayStats;
    bool day_stats_pending;
    bool day_requested_today;

    RefuelRecord lastRefuel;

    struct {
        TripStats data;
        uint8_t   expected_count;
        uint8_t   received_count;
        bool      active;
    } trip_rx;

    struct {
        DayStats data;
        uint8_t  expected_count;
        uint8_t  received_count;
        bool     active;
    } day_rx;

    /* ================ ЗАЕЗДЫ (DRIVE CYCLES) ================ */
    DriveCycle receivedDriveCycles[15]; ///< Массив принятых от шлюза заездов
    uint8_t    receivedDriveCycleCount; ///< Реальное количество заездов
    bool       drive_cycles_rx_active;  ///< Флаг активного приёма заездов
    bool       drive_cycles_pending;    ///< Флаг ожидания приёма заездов

    /* ================ ФЛАГ СИНХРОНИЗАЦИИ ВРЕМЕНИ ================ */
    bool time_synced;
    bool time_received_this_boot; 
    
    /* ================ ЯРКОСТЬ ================ */
    uint8_t  backlight_brightness;      // Ручная яркость (0-100%)
    uint8_t  light_threshold_dark;      // Порог "темно"
    uint8_t  light_threshold_bright;    // Порог "светло"
    uint8_t  backlight_min_brightness;  // Минимальная яркость (0-100%)
    uint8_t  ambient_light_pct;         // Освещённость от физического датчика (0-100)
    uint8_t  ambient_light_synth;       // Синтетическая освещённость по времени (0-100)
    uint16_t ambient_light_raw;         // Сырое значение АЦП фоторезистора (0–4095)

    /* ================ ТЕМА ФОНА ================ */
    uint8_t  bg_theme;                  // 0 = синий (по умолчанию), 1 = зелёный, 2 = красный

    /* ================ РУЧНАЯ УСТАНОВКА ВРЕМЕНИ ================ */
    bool time_set_manually;             // true - ручная установка имеет приоритет

    /* ================ ТЕПЛОВАЯ ПАМЯТЬ ================ */
    int last_valid_coolant_temp;        // Последнее достоверное значение температуры ОЖ

    /* ================ ЦВЕТА ТОНАЛЬНОСТЕЙ ================ */
    uint32_t color_tone_neutral;        // Бирюзовый по умолчанию
    uint32_t color_tone_funny;          // Мягкий розовый
    uint32_t color_tone_serious;        // Зелёный

} CarData;

/* ============================================================================
 *                      ПРОТОТИПЫ ФУНКЦИЙ
 * ========================================================================== */

void CarData_init(CarData* data);

uint32_t CarData_getFuelColor(const CarData* data);
uint32_t CarData_getBatteryColor(const CarData* data);
uint32_t CarData_getTempColor(const CarData* data);
uint32_t CarData_getSpeedColor(const CarData* data);
uint32_t CarData_getRPMColor(const CarData* data);
uint32_t CarData_getLPHColor(const CarData* data);

void CarData_formatTimeHMS(uint32_t totalSeconds, char* buffer, size_t bufferSize);

bool CarData_Init(void);
CarData* CarData_Get(void);
bool CarData_Lock(uint32_t timeout_ms);
void CarData_Unlock(void);

void CarData_set_time_synced(bool synced);

#ifdef __cplusplus
}
#endif

#endif /* CAR_DATA_H */