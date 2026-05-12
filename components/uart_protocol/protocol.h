/**
 * @file protocol.h
 * @brief Общие константы бинарного протокола UART.
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
#define MSG_DRIVE_CYCLE_DATA    0x4F  // первый пакет с количеством заездов
// Поля заездов: коды от 0x50 до 0x5C (13 полей)
#define MSG_DRIVE_CYCLE_END     0x66  // завершение передачи заездов (исправлено)

// ------------------ HEARTBEAT И СЛУЖЕБНЫЕ ------------------
#define MSG_HEARTBEAT_REQ      0x60   // теперь приходит с 4 байтами Unix-времени
#define MSG_HEARTBEAT_RSP      0x61
#define MSG_CONFIG_REQ         0x62
#define MSG_CONFIG_RSP         0x63

// ------------------ ОБНАРУЖЕНИЕ УСТРОЙСТВ -----------------
#define MSG_WHO_IS_HERE        0x70
#define MSG_I_AM_HERE          0x71

// Типы устройств
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
#define MSG_REFUEL_DATA        0x50   // заправка (не конфликтует с полями заездов 0x50..0x5C)
#define MSG_ODO_FULL           0x51
#define MSG_FULL_TANK_FLAG     0x52
#define MSG_SET_FUEL_LEVEL     0x53
#define MSG_SET_ODO            0x54
#define MSG_REBOOT             0x64

// Датчик освещённости
#define MSG_LIGHT            0x95   // запрос (Len=0) и ответ (Len=1, 0-100%)
#define MSG_LIGHT_SYNTH      0x96   // синтетическая освещённость по времени суток (Len=1, 0-100%)
#define MSG_LIGHT_RAW        0x97   // запрос (Len=0) и ответ (Len=2, uint16_t 0..4095)

uint8_t crc8_calculate(const uint8_t *data, size_t len);

#endif // PROTOCOL_H