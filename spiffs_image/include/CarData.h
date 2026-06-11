/**
 * @file CarData.h
 * @brief Единая структура данных автомобиля и распределённой системы «Фиона».
 *        Добавлен мьютекс для синхронизации доступа из разных задач.
 *        Топливо теперь хранится в миллилитрах (fuelValueML) для целочисленной точности.
 */

#ifndef CAR_DATA_H
#define CAR_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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
 *                          ЗАЕЗД (Drive Cycle)
 * ========================================================================== */
typedef struct {
    uint32_t startTime;
    uint32_t endTime;
    uint32_t duration;
    uint32_t distance;          // метры
    float    fuelUsed;          // литры (оставлено float для совместимости с историей)
    uint16_t maxSpeed;
    uint16_t maxLPH;
    uint16_t avgThrottleRel;
    uint16_t maxThrottleRel;
    uint16_t aggressiveCount;
    uint16_t fullThrottleCount;
    uint32_t warmupSeconds;
} DriveCycle;

/* ============================================================================
 *                          ОСНОВНАЯ СТРУКТУРА ДАННЫХ
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
    float    throttleRelPos;
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
    bool throttleRelDirty;
    bool stftDirty;
    bool ltftDirty;
    bool fuelPressureDirty;

    /* ================ КАТЕГОРИЯ 2: ВЫЧИСЛЯЕМЫЕ ШЛЮЗОМ ЗНАЧЕНИЯ ================ */
    float    lphValue;
    uint32_t fuelValueML;       // Текущий остаток в миллилитрах (0..50000)
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

    /* ------------------ Калибровка топлива (внутренние) ------------------ */
    float    fuelPrice;
    float    fuelCalibrationFactor;
    uint32_t lastFullOdoKm;
    float    totalRefuelSinceLastFull;   // литры, для совместимости с историей
    float    calculatedFuelSinceLastFull;
    bool     hasFirstFullTank;
    uint32_t initialOdoKm;
    float    initialFuel;
    bool     calibrationNeeded;

    /* ------------------ Параметры логики поездок ------------------------- */
    uint16_t tripAutoStopTimeout;
    uint32_t lastManualStopTime;
    uint16_t sleepTimeout;

    /* ================ КАТЕГОРИЯ 3: ДАННЫЕ ОТ ARDUINO (NTC и исполнители) ====== */
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

    /* ---------- Новые поля для дублирующего управления климатом ---------- */
    bool    fanControlEnabled;
    bool    arduinoPresent;
    uint8_t fanMode;

    uint8_t fan1CalibStartPoint;
    uint8_t fan1CalibStopPoint;
    uint8_t fan1NoiseLow;
    uint8_t fan1NoiseHigh;

    uint8_t fan2CalibStartPoint;
    uint8_t fan2CalibStopPoint;
    uint8_t fan2NoiseLow;
    uint8_t fan2NoiseHigh;

    uint8_t climatePreset;
    uint8_t climateCalibNoiseLow;
    uint8_t climateCalibNoiseHigh;
    uint8_t climateCalibStartPoint;
    uint8_t climateCalibStopPoint;

    float   coolantTempOffset;

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
    bool uartArduinoAlive;
    bool uartEsp32Alive;
    int8_t wifiRSSI;
    bool wifiRSSIDirty;

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
    uint32_t systemSyncTime;
    uint32_t lastObdUpdate;
    uint32_t lastPresetUpdate;
    uint32_t lastApiResponse;

    /* ================ СЛУЖЕБНЫЕ ФЛАГИ ================ */
    bool configDirty;
    bool requestConfigSync;

    /* ================ СТАТИСТИКА ПОЕЗДКИ (РЕЙСА) ================ */
    uint32_t tripStatStartTime;
    bool     tripStatIsManual;
    uint32_t tripStatDuration;
    uint32_t tripStatPauseTime;
    uint16_t tripStatPauseCount;
    uint32_t tripStatDistance;
    float    tripStatFuelUsed;
    uint16_t tripStatMaxSpeed;
    uint16_t tripStatMaxLPH;
    uint16_t tripStatAvgThrottleRel;
    uint16_t tripStatMaxThrottleRel;
    uint32_t tripStatWarmupSeconds;
    uint16_t tripStatAggressiveCount;
    uint16_t tripStatFullThrottleCount;
    uint32_t tripStatDriveTime;

    bool tripStatPending;
    bool tripStatsSendRequested;

    /* ================ СУТОЧНАЯ СТАТИСТИКА ================ */
    uint32_t dayStatDate;
    bool     dayStatValid;
    uint32_t dayStatEngineSeconds;
    uint32_t dayStatDistance;
    float    dayStatFuelUsed;
    uint16_t dayStatMaxSpeed;
    uint16_t dayStatMaxLPH;
    uint16_t dayStatAvgThrottleRel;
    uint16_t dayStatMaxThrottleRel;
    uint32_t dayStatWarmupSeconds;
    uint16_t dayStatAggressiveCount;
    uint16_t dayStatFullThrottleCount;
    uint32_t dayStatDriveTime;
    uint32_t dayStatFirstStart;
    uint32_t dayStatLastStop;
    uint8_t  dayStatTripCount;
    uint8_t  dayStatDriveCycleCount;

    bool dayStatPending;
    bool dayStatsSendRequested;

    /* ================ МАССИВ ЗАВЕРШЁННЫХ ЗАЕЗДОВ ================ */
    DriveCycle driveCycles[15];
    uint8_t    driveCycleCount;

    /* ---------- Внутренние счётчики (не сохраняются) ---------- */
    uint32_t tripStartTime;
    uint32_t tripEngineSeconds;
    uint16_t tripMaxSpeed;
    uint16_t tripMaxLPH;
    uint32_t tripAvgThrottleRelAccum;
    uint16_t tripMaxThrottleRel;
    uint8_t  tripPauseCounter;
    uint32_t tripWarmupSeconds;
    uint16_t tripAggressiveCount;
    uint16_t tripFullThrottleCount;
    uint32_t tripDriveTime;

    uint32_t dayAccEngineSeconds;
    uint32_t dayAccDistance;
    float    dayAccFuelUsed;
    uint16_t dayAccMaxSpeed;
    uint16_t dayAccMaxLPH;
    uint32_t dayAccAvgThrottleRelAccum;
    uint16_t dayAccMaxThrottleRel;
    uint32_t dayAccWarmupSeconds;
    uint16_t dayAccAggressiveCount;
    uint16_t dayAccFullThrottleCount;
    uint32_t dayAccDriveTime;
    uint32_t dayAccDate;
    uint32_t dayFirstStart;
    uint32_t dayLastStop;
    uint8_t  dayTripCounter;

    // Данные акселерометра (публичные для отправки)
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    float tiltRoll, tiltPitch;
    uint16_t imuAggressiveAccel;
    uint16_t imuAggressiveBrake;
    uint16_t imuAggressiveCorner;
    bool imuCalibrated;

} CarData;

/* ============================================================================
 *                      ПРОТОТИПЫ ФУНКЦИЙ
 * ========================================================================== */

/**
 * @brief Инициализирует структуру CarData значениями по умолчанию.
 * @param data Указатель на уже выделенную память структуры.
 */
void CarData_init(CarData* data);

/**
 * @brief Получить цвет для текущего уровня топлива.
 */
uint32_t CarData_getFuelColor(const CarData* data);
/**
 * @brief Получить цвет для напряжения.
 */
uint32_t CarData_getBatteryColor(const CarData* data);
/**
 * @brief Получить цвет для температуры ОЖ.
 */
uint32_t CarData_getTempColor(const CarData* data);
/**
 * @brief Получить цвет для скорости.
 */
uint32_t CarData_getSpeedColor(const CarData* data);
/**
 * @brief Получить цвет для оборотов.
 */
uint32_t CarData_getRPMColor(const CarData* data);
/**
 * @brief Получить цвет для мгновенного расхода.
 */
uint32_t CarData_getLPHColor(const CarData* data);

/**
 * @brief Форматирует время в строку ЧЧ:ММ:СС.
 */
void CarData_formatTimeHMS(uint32_t totalSeconds, char* buffer, size_t bufferSize);

/**
 * @brief Захватывает мьютекс для монопольного доступа к CarData.
 * @param timeout_ms Максимальное время ожидания в миллисекундах (0 - не ждать).
 * @return true если мьютекс взят.
 */
bool CarData_Lock(uint32_t timeout_ms);

/**
 * @brief Освобождает мьютекс CarData.
 */
void CarData_Unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* CAR_DATA_H */