#include "UARTBinary.h"
#include "Protocol.h"
#include "CarData.h"
#include <Arduino.h>

extern CarData carData;

static uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x01) crc = (crc >> 1) ^ 0x8C;
            else crc >>= 1;
        }
    }
    return crc;
}

static uart_command_callback_t cmd_callback = NULL;

#define RX_BUF_SIZE 16
static uint8_t rx_buffer[RX_BUF_SIZE];
static size_t rx_index = 0;
static bool in_frame = false;

bool waitingTripAck = false;
uint32_t tripAckStart = 0;
bool waitingDayAck = false;
uint32_t dayAckStart = 0;
bool waitingDriveAck = false;
uint32_t driveAckStart = 0;

void uart_binary_init(unsigned long baud_rate, int rx_pin, int tx_pin) {
    Serial2.begin(baud_rate, SERIAL_8N1, rx_pin, tx_pin);
}

void uart_send_packet(uint8_t msg_id, uint8_t src, uint8_t dst, const uint8_t* payload, uint8_t len) {
    if (len > 4) len = 4;
    uint8_t frame[FRAME_TOTAL_SIZE];
    frame[0] = FRAME_MAGIC;
    frame[1] = msg_id;
    frame[2] = src;
    frame[3] = dst;
    frame[4] = len;
    for (int i = 0; i < 4; i++) frame[5 + i] = (i < len) ? payload[i] : 0;
    uint8_t crc = crc8(frame, 9);
    frame[9] = crc;
    Serial2.write(frame, FRAME_TOTAL_SIZE);
}

static void process_frame(const uint8_t* frame) {
    uint8_t crc_calc = crc8(frame, 9);
    if (crc_calc != frame[9]) { Serial.println("[UART] CRC error"); return; }
    uint8_t msg_id = frame[1];
    uint8_t src = frame[2];
    uint8_t dst = frame[3];
    uint8_t len = frame[4];
    if (dst != ADDR_ESP32_GW && dst != ADDR_BROADCAST) return;
    const uint8_t* payload = &frame[5];
    if (cmd_callback) cmd_callback(msg_id, payload, len);
}

/**
 * @brief Отправка статистики завершённой поездки.
 *        Все поля читаются атомарно под мьютексом.
 */
void sendTripStats() {
    if (!CarData_Lock(100)) return;

    uint32_t t_start = carData.tripStatStartTime;
    bool isManual = carData.tripStatIsManual;
    uint32_t dur = carData.tripStatDuration;
    uint32_t pause = carData.tripStatPauseTime;
    uint16_t pauseCnt = carData.tripStatPauseCount;
    uint32_t dist = carData.tripStatDistance;
    float fuelUsed = carData.tripStatFuelUsed;
    uint16_t maxSpeed = carData.tripStatMaxSpeed;
    uint16_t maxLPH = carData.tripStatMaxLPH;
    uint32_t warmup = carData.tripStatWarmupSeconds;
    uint16_t avgThrottle = carData.tripStatAvgThrottleRel;
    uint16_t maxThrottle = carData.tripStatMaxThrottleRel;
    uint16_t aggr = carData.tripStatAggressiveCount;
    uint16_t fullThr = carData.tripStatFullThrottleCount;
    uint32_t driveTime = carData.tripStatDriveTime;
    CarData_Unlock();

    uint8_t count = 16;
    uart_send_packet(MSG_TRIP_STATS_START, ADDR_ESP32_GW, ADDR_ESP32_P4, &count, 1);

    uint8_t buf[4];
    buf[0]=t_start&0xFF; buf[1]=(t_start>>8)&0xFF; buf[2]=(t_start>>16)&0xFF; buf[3]=(t_start>>24)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_START_TIME, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uint8_t st = isManual ? 1 : 0;
    uart_send_packet(MSG_TRIP_STAT_STATUS, ADDR_ESP32_GW, ADDR_ESP32_P4, &st, 1);

    buf[0]=dur&0xFF; buf[1]=(dur>>8)&0xFF; buf[2]=(dur>>16)&0xFF; buf[3]=(dur>>24)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_DURATION, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    buf[0]=pause&0xFF; buf[1]=(pause>>8)&0xFF; buf[2]=(pause>>16)&0xFF; buf[3]=(pause>>24)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_PAUSE_TIME, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uint8_t b2[2];
    b2[0]=pauseCnt&0xFF; b2[1]=(pauseCnt>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_PAUSE_CNT, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    buf[0]=dist&0xFF; buf[1]=(dist>>8)&0xFF; buf[2]=(dist>>16)&0xFF; buf[3]=(dist>>24)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_DIST, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uint16_t fuel = (uint16_t)(fuelUsed * 100.0f);
    b2[0]=fuel&0xFF; b2[1]=(fuel>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_FUEL, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=maxSpeed&0xFF; b2[1]=(maxSpeed>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_MAX_SPEED, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=maxLPH&0xFF; b2[1]=(maxLPH>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_MAX_LPH, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    buf[0]=warmup&0xFF; buf[1]=(warmup>>8)&0xFF; buf[2]=(warmup>>16)&0xFF; buf[3]=(warmup>>24)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_WARMUP, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    b2[0]=avgThrottle&0xFF; b2[1]=(avgThrottle>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_AVG_THROTTLE, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=maxThrottle&0xFF; b2[1]=(maxThrottle>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_MAX_THROTTLE, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=aggr&0xFF; b2[1]=(aggr>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_AGGR_COUNT, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=fullThr&0xFF; b2[1]=(fullThr>>8)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_FULL_THROTTLE, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    buf[0]=driveTime&0xFF; buf[1]=(driveTime>>8)&0xFF; buf[2]=(driveTime>>16)&0xFF; buf[3]=(driveTime>>24)&0xFF;
    uart_send_packet(MSG_TRIP_STAT_DRIVE_TIME, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uart_send_packet(MSG_TRIP_STATS_END, ADDR_ESP32_GW, ADDR_ESP32_P4, NULL, 0);
}

/**
 * @brief Отправка суточной статистики с защитой чтения.
 */
void sendDayStats() {
    if (!CarData_Lock(100)) return;

    uint32_t date = carData.dayStatDate;
    bool valid = carData.dayStatValid;
    uint32_t engSec = carData.dayStatEngineSeconds;
    uint32_t dist = carData.dayStatDistance;
    float fuelUsed = carData.dayStatFuelUsed;
    uint16_t maxSpeed = carData.dayStatMaxSpeed;
    uint16_t maxLPH = carData.dayStatMaxLPH;
    uint32_t firstStart = carData.dayStatFirstStart;
    uint32_t lastStop = carData.dayStatLastStop;
    uint8_t tripCnt = carData.dayStatTripCount;
    uint8_t dcCnt = carData.dayStatDriveCycleCount;
    uint16_t avgThr = carData.dayStatAvgThrottleRel;
    uint16_t maxThr = carData.dayStatMaxThrottleRel;
    uint32_t warmup = carData.dayStatWarmupSeconds;
    uint16_t aggr = carData.dayStatAggressiveCount;
    uint16_t fullThr = carData.dayStatFullThrottleCount;
    uint32_t driveTime = carData.dayStatDriveTime;
    CarData_Unlock();

    uint8_t count = 18;
    uart_send_packet(MSG_DAY_STATS_START, ADDR_ESP32_GW, ADDR_ESP32_P4, &count, 1);

    uint8_t buf[4];
    buf[0]=date&0xFF; buf[1]=(date>>8)&0xFF; buf[2]=(date>>16)&0xFF; buf[3]=(date>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_DATE, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uint8_t v = valid ? 1 : 0;
    uart_send_packet(MSG_DAY_STAT_VALID, ADDR_ESP32_GW, ADDR_ESP32_P4, &v, 1);

    buf[0]=engSec&0xFF; buf[1]=(engSec>>8)&0xFF; buf[2]=(engSec>>16)&0xFF; buf[3]=(engSec>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_ENG_SEC, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    buf[0]=dist&0xFF; buf[1]=(dist>>8)&0xFF; buf[2]=(dist>>16)&0xFF; buf[3]=(dist>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_DIST, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uint16_t fuel = (uint16_t)(fuelUsed * 100.0f);
    uint8_t b2[2]; b2[0]=fuel&0xFF; b2[1]=(fuel>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_FUEL, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=maxSpeed&0xFF; b2[1]=(maxSpeed>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_MAX_SPEED, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=maxLPH&0xFF; b2[1]=(maxLPH>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_MAX_LPH, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    buf[0]=firstStart&0xFF; buf[1]=(firstStart>>8)&0xFF; buf[2]=(firstStart>>16)&0xFF; buf[3]=(firstStart>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_FIRST_START, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    buf[0]=lastStop&0xFF; buf[1]=(lastStop>>8)&0xFF; buf[2]=(lastStop>>16)&0xFF; buf[3]=(lastStop>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_LAST_STOP, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uart_send_packet(MSG_DAY_STAT_TRIP_COUNT, ADDR_ESP32_GW, ADDR_ESP32_P4, &tripCnt, 1);

    uart_send_packet(MSG_DAY_STAT_DC_COUNT, ADDR_ESP32_GW, ADDR_ESP32_P4, &dcCnt, 1);

    b2[0]=avgThr&0xFF; b2[1]=(avgThr>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_AVG_THROTTLE, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=maxThr&0xFF; b2[1]=(maxThr>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_MAX_THROTTLE, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    buf[0]=warmup&0xFF; buf[1]=(warmup>>8)&0xFF; buf[2]=(warmup>>16)&0xFF; buf[3]=(warmup>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_WARMUP, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    b2[0]=aggr&0xFF; b2[1]=(aggr>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_AGGR_COUNT, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    b2[0]=fullThr&0xFF; b2[1]=(fullThr>>8)&0xFF;
    uart_send_packet(MSG_DAY_STAT_FULL_THROTTLE, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

    buf[0]=driveTime&0xFF; buf[1]=(driveTime>>8)&0xFF; buf[2]=(driveTime>>16)&0xFF; buf[3]=(driveTime>>24)&0xFF;
    uart_send_packet(MSG_DAY_STAT_DRIVE_TIME, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

    uart_send_packet(MSG_DAY_STATS_END, ADDR_ESP32_GW, ADDR_ESP32_P4, NULL, 0);
}

/**
 * @brief Отправка массива завершённых заездов.
 */
void sendDriveCycles() {
    if (!CarData_Lock(100)) return;

    uint8_t cnt = carData.driveCycleCount;
    // Копируем массив заездов, чтобы не держать мьютекс долго
    DriveCycle cycles[15];
    memcpy(cycles, carData.driveCycles, sizeof(DriveCycle) * cnt);
    CarData_Unlock();

    uart_send_packet(MSG_DRIVE_CYCLE_DATA, ADDR_ESP32_GW, ADDR_ESP32_P4, &cnt, 1);
    for (uint8_t i = 0; i < cnt; i++) {
        DriveCycle* dc = &cycles[i];
        uint8_t buf[4];

        buf[0]=dc->startTime&0xFF; buf[1]=(dc->startTime>>8)&0xFF; buf[2]=(dc->startTime>>16)&0xFF; buf[3]=(dc->startTime>>24)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+1, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

        buf[0]=dc->endTime&0xFF; buf[1]=(dc->endTime>>8)&0xFF; buf[2]=(dc->endTime>>16)&0xFF; buf[3]=(dc->endTime>>24)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+2, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

        buf[0]=dc->duration&0xFF; buf[1]=(dc->duration>>8)&0xFF; buf[2]=(dc->duration>>16)&0xFF; buf[3]=(dc->duration>>24)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+3, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

        buf[0]=dc->distance&0xFF; buf[1]=(dc->distance>>8)&0xFF; buf[2]=(dc->distance>>16)&0xFF; buf[3]=(dc->distance>>24)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+4, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);

        uint16_t fuel = (uint16_t)(dc->fuelUsed * 100.0f);
        uint8_t b2[2]; b2[0]=fuel&0xFF; b2[1]=(fuel>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+5, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        b2[0]=dc->maxSpeed&0xFF; b2[1]=(dc->maxSpeed>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+6, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        b2[0]=dc->maxLPH&0xFF; b2[1]=(dc->maxLPH>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+7, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        b2[0]=dc->avgThrottleRel&0xFF; b2[1]=(dc->avgThrottleRel>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+8, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        b2[0]=dc->maxThrottleRel&0xFF; b2[1]=(dc->maxThrottleRel>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+9, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        b2[0]=dc->aggressiveCount&0xFF; b2[1]=(dc->aggressiveCount>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+10, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        b2[0]=dc->fullThrottleCount&0xFF; b2[1]=(dc->fullThrottleCount>>8)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+11, ADDR_ESP32_GW, ADDR_ESP32_P4, b2, 2);

        buf[0]=dc->warmupSeconds&0xFF; buf[1]=(dc->warmupSeconds>>8)&0xFF; buf[2]=(dc->warmupSeconds>>16)&0xFF; buf[3]=(dc->warmupSeconds>>24)&0xFF;
        uart_send_packet(MSG_DRIVE_CYCLE_DATA+12, ADDR_ESP32_GW, ADDR_ESP32_P4, buf, 4);
    }
    uart_send_packet(MSG_DRIVE_CYCLES_END, ADDR_ESP32_GW, ADDR_ESP32_P4, NULL, 0);
}

void uart_task_process(void) {
    static uint32_t last_heartbeat_time = 0;
    uint32_t now = millis();

    // Приём байтов и сбор кадров
    while (Serial2.available()) {
        uint8_t b = Serial2.read();
        if (!in_frame) {
            if (b == FRAME_MAGIC) {
                rx_index = 0;
                rx_buffer[rx_index++] = b;
                in_frame = true;
            }
        } else {
            rx_buffer[rx_index++] = b;
            if (rx_index >= FRAME_TOTAL_SIZE) {
                process_frame(rx_buffer);
                in_frame = false;
                rx_index = 0;
            }
        }
    }

    // Таймауты ACK
    if (waitingTripAck && (now - tripAckStart > 10000)) waitingTripAck = false;
    if (waitingDayAck && (now - dayAckStart > 10000)) waitingDayAck = false;
    if (waitingDriveAck && (now - driveAckStart > 10000)) waitingDriveAck = false;

    // Периодическая отправка heartbeat раз в 5 секунд
    if (now - last_heartbeat_time >= 5000) {
        last_heartbeat_time = now;
        uint32_t t;
        if (CarData_Lock(10)) {
            t = carData.systemTime;
            CarData_Unlock();
        } else {
            t = 0;
        }
        uint8_t timePayload[4];
        timePayload[0] = t & 0xFF; timePayload[1] = (t>>8)&0xFF; timePayload[2] = (t>>16)&0xFF; timePayload[3] = (t>>24)&0xFF;
        uart_send_packet(MSG_HEARTBEAT_REQ, ADDR_ESP32_GW, ADDR_ESP32_P4, timePayload, 4);
    }

    // Информирование об изменении статуса OBD
    static bool last_obd = false;
    bool cur_obd;
    if (CarData_Lock(10)) {
        cur_obd = carData.obdConnected;
        CarData_Unlock();
    } else {
        cur_obd = last_obd;
    }
    if (cur_obd != last_obd) {
        last_obd = cur_obd;
        uint8_t p = cur_obd ? 1 : 0;
        uart_send_packet(MSG_OBD_STATUS, ADDR_ESP32_GW, ADDR_ESP32_P4, &p, 1);
    }
}

void uart_set_command_callback(uart_command_callback_t callback) {
    cmd_callback = callback;
}