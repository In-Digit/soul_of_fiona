/**
 * @file protocol.h
 * @brief Общие константы бинарного протокола UART.
 * Добавлены недостающие ID для статистики, ACK и IMU.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// Адреса устройств
#define ADDR_ARDUINO     0x01
#define ADDR_ESP32_GW    0x02
#define ADDR_ESP32_P4    0x03
#define ADDR_BROADCAST   0x00

// Magic byte для синхронизации кадра
#define FRAME_MAGIC      0xAA

// Размеры кадра
#define FRAME_HEADER_SIZE  6
#define FRAME_PAYLOAD_SIZE 4
#define FRAME_TOTAL_SIZE   (FRAME_HEADER_SIZE + FRAME_PAYLOAD_SIZE)

// ---------------------- ДАННЫЕ ОТ ШЛЮЗА (0x20 – 0x32) ---------------------
#define MSG_MAF              0x20
#define MSG_SPEED            0x21
#define MSG_RPM              0x22
#define MSG_COOLANT_TEMP     0x23
#define MSG_VOLTAGE          0x24
#define MSG_OBD_STATUS       0x25
#define MSG_TRIP_TIME        0x26
#define MSG_TRIP_PAUSE       0x27
#define MSG_TRIP_COST        0x28
#define MSG_TRIP_FUEL        0x29
#define MSG_FUEL_LEVEL       0x2A
#define MSG_RANGE            0x2B
#define MSG_INST_FUEL        0x2C
#define MSG_ODO              0x2D
#define MSG_TRIP_STATUS      0x2E
#define MSG_TRIP_TOGGLE      0x2F
#define MSG_TRIP_DIST        0x30
#define MSG_THROTTLE         0x31   // абсолютное положение дросселя
#define MSG_THROTTLE_REL     0x32   // относительное положение дросселя

// ------------------ СТАТИСТИКА ПОЕЗДОК (рейс) --------------------
#define MSG_TRIP_STATS_START     0x35
#define MSG_TRIP_STAT_START_TIME 0x36
#define MSG_TRIP_STAT_STATUS     0x37
#define MSG_TRIP_STAT_DURATION   0x38
#define MSG_TRIP_STAT_PAUSE_TIME 0x39
#define MSG_TRIP_STAT_PAUSE_CNT  0x3A
#define MSG_TRIP_STAT_DIST       0x3B
#define MSG_TRIP_STAT_FUEL       0x3C
#define MSG_TRIP_STAT_MAX_SPEED  0x3D
#define MSG_TRIP_STAT_MAX_LPH    0x3E
#define MSG_TRIP_STATS_END       0x3F
// Новые поля рейса
#define MSG_TRIP_STAT_WARMUP            0x55
#define MSG_TRIP_STAT_AVG_THROTTLE_REL  0x56
#define MSG_TRIP_STAT_MAX_THROTTLE_REL  0x57
#define MSG_TRIP_STAT_AGGRESSIVE_COUNT  0x58
#define MSG_TRIP_STAT_FULL_THROTTLE_CNT 0x59
#define MSG_TRIP_STAT_MOVING_TIME       0x5A

// ------------------ СУТОЧНАЯ СТАТИСТИКА --------------------
#define MSG_DAY_STATS_START     0x42
#define MSG_DAY_STAT_DATE       0x43
#define MSG_DAY_STAT_VALID      0x44
#define MSG_DAY_STAT_ENG_SEC    0x45
#define MSG_DAY_STAT_DIST       0x46
#define MSG_DAY_STAT_FUEL       0x47
#define MSG_DAY_STAT_MAX_SPEED  0x48
#define MSG_DAY_STAT_MAX_LPH    0x49
#define MSG_DAY_STATS_END       0x4A
// Новые поля суточной статистики
#define MSG_DAY_STAT_FIRST_START        0x5B
#define MSG_DAY_STAT_LAST_STOP          0x5C
#define MSG_DAY_STAT_TRIP_COUNT         0x5D
#define MSG_DAY_STAT_DRIVE_COUNT        0x5E
#define MSG_DAY_STAT_AVG_THROTTLE_REL   0x5F
#define MSG_DAY_STAT_MAX_THROTTLE_REL   0x60
#define MSG_DAY_STAT_WARMUP             0x61
#define MSG_DAY_STAT_AGGRESSIVE_COUNT   0x62
#define MSG_DAY_STAT_FULL_THROTTLE_CNT  0x63
#define MSG_DAY_STAT_MOVING_TIME        0x64

// ------------------ ПОДТВЕРЖДЕНИЯ --------------------------
#define MSG_TRIP_STATS_ACK      0x41
#define MSG_DAY_STATS_ACK       0x4B
#define MSG_DRIVE_CYCLES_ACK    0x65

// ------------------ ЗАПРОСЫ СТАТИСТИКИ --------------------
#define MSG_REQ_TRIP_STATS      0x4C
#define MSG_REQ_DAY_STATS       0x4D
#define MSG_REQ_DRIVE_CYCLES    0x4E
#define MSG_DRIVE_CYCLE_DATA    0x4F
#define MSG_DRIVE_CYCLE_END     0x66

// ------------------ HEARTBEAT И СЛУЖЕБНЫЕ ------------------
#define MSG_HEARTBEAT_REQ      0x60
#define MSG_HEARTBEAT_RSP      0x61
#define MSG_CONFIG_REQ         0x62
#define MSG_CONFIG_RSP         0x63

// ------------------ ОБНАРУЖЕНИЕ УСТРОЙСТВ -----------------
#define MSG_WHO_IS_HERE        0x70
#define MSG_I_AM_HERE          0x71

#define DEV_TYPE_GATEWAY       0x01
#define DEV_TYPE_ARDUINO       0x02

// ------------------ СИНХРОНИЗАЦИЯ ВРЕМЕНИ -----------------
#define MSG_REQ_TIME           0x90
#define MSG_TIME               0x91

// ------------------ API (ЗАГЛУШКИ) -------------------------
#define MSG_REQ_API            0x92
#define MSG_API_RESP           0x93

// ------------------ ИНТЕРНЕТ СИНХРОНИЗАЦИЯ -----------------
#define MSG_INTERNET_SYNC      0x94

// ------------------ УПРАВЛЕНИЕ ШЛЮЗОМ ----------------------
#define MSG_RECONNECT          0x40
#define MSG_REFUEL_DATA        0x50
#define MSG_ODO_FULL           0x51
#define MSG_FULL_TANK_FLAG     0x52
#define MSG_SET_FUEL_LEVEL     0x53
#define MSG_SET_ODO            0x54
#define MSG_REBOOT             0x64

// Датчик освещённости
#define MSG_LIGHT            0x95
#define MSG_LIGHT_SYNTH      0x96
#define MSG_LIGHT_RAW        0x97

// --------------- IMU (АКСЕЛЕРОМЕТР / ГИРОСКОП / НАКЛОН) ---------------
#define MSG_REQ_ACCEL         0xA0
#define MSG_ACCEL_Z           0xA5
#define MSG_REQ_GYRO          0xA1
#define MSG_REQ_TILT          0xA2
#define MSG_CALIBRATE_ACCEL   0xA3
#define MSG_REQ_CALIB_STATUS  0xA4

// Управление вентиляторами и климатом
#define MSG_FAN_SET_MODE           0xC0
#define MSG_FAN_SET_PWM1           0xC1
#define MSG_FAN_SET_PWM2           0xC2
#define MSG_FAN_SET_AUTO           0xC3
#define MSG_CLIMATE_SET_PRESET     0xC4
#define MSG_CLIMATE_SET_TEMP       0xC5
#define MSG_CLIMATE_SET_PWM        0xC6
#define MSG_CLIMATE_SET_AUTO       0xC7

// Телеметрия климата (запрос/ответ)
#define MSG_FAN_TELEMETRY          0xC8
#define MSG_CLIMATE_TELEMETRY      0xC9

// Калибровка вентиляторов радиатора
#define MSG_FAN_CALIB_START        0xCA
#define MSG_FAN_CALIB_STEP         0xCB
#define MSG_FAN_CALIB_START_POINT  0xCC
#define MSG_FAN_CALIB_STOP_POINT   0xCD
#define MSG_FAN_CALIB_NOISE_LOW    0xCE
#define MSG_FAN_CALIB_NOISE_HIGH   0xCF
#define MSG_FAN_CALIB_SAVE         0xD0

// Калибровка печки
#define MSG_HEATER_CALIB_START       0xD1
#define MSG_HEATER_CALIB_STEP        0xD2
#define MSG_HEATER_CALIB_START_POINT 0xD3
#define MSG_HEATER_CALIB_STOP_POINT  0xD4
#define MSG_HEATER_CALIB_NOISE_LOW   0xD5
#define MSG_HEATER_CALIB_NOISE_HIGH  0xD6
#define MSG_HEATER_CALIB_SAVE        0xD7

// Температурное смещение Arduino
#define MSG_TEMP_OFFSET_SET   0xE0
#define MSG_TEMP_OFFSET_GET   0xE1

// Запрос стиля вождения у шлюза
#define MSG_REQ_DRIVING_STYLE 0xE2

uint8_t crc8_calculate(const uint8_t *data, size_t len);

#endif // PROTOCOL_H