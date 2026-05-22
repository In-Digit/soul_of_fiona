/**
 * @file uart_rx_arduino.c
 * @brief Обработчик данных от Arduino (вентиляторы радиатора, температура).
 *
 * Принимает и разбирает телеметрию: ШИМ вентиляторов, температуру ОЖ,
 * режим работы (автономный/от экрана).
 */

#include "uart_rx_arduino.h"
#include "protocol.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "esp_log.h"

static const char *TAG = "UART_ARDUINO";

void uart_rx_arduino_process(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    CarData *data = CarData_Get();
    if (!data) return;

    // Любой валидный пакет от Arduino — признак жизни
    CarData_Lock(10);
    data->uartArduinoAlive = true;
    CarData_Unlock();

    switch (msg_id) {
        case MSG_FAN_TELEMETRY:   // 0xC8 — телеметрия вентиляторов
            if (len >= 4) {
                uint8_t fan1 = payload[0];
                uint8_t fan2 = payload[1];
                int16_t temp_raw = (int16_t)(payload[2] | (payload[3] << 8));
                float temp = temp_raw / 10.0f;

                CarData_Lock(10);
                data->fanCurrentPWM1 = fan1;
                data->fanCurrentPWM2 = fan2;
                data->arduino_coolant_temp = temp;
                data->arduino_coolant_dirty = true;
                data->fan1Dirty = true;
                data->fan2Dirty = true;
                CarData_Unlock();
            }
            break;

        case MSG_HEARTBEAT_RSP:    // 0x61 — heartbeat от Arduino
            // Формат такой же, как MSG_FAN_TELEMETRY (fan1, fan2, temp_x10)
            if (len >= 4) {
                uint8_t fan1 = payload[0];
                uint8_t fan2 = payload[1];
                int16_t temp_raw = (int16_t)(payload[2] | (payload[3] << 8));
                float temp = temp_raw / 10.0f;

                CarData_Lock(10);
                data->fanCurrentPWM1 = fan1;
                data->fanCurrentPWM2 = fan2;
                data->arduino_coolant_temp = temp;
                data->arduino_coolant_dirty = true;
                data->fan1Dirty = true;
                data->fan2Dirty = true;
                // Режим Arduino не передаётся в heartbeat, оставляем предыдущий
                CarData_Unlock();
            }
            break;

        case MSG_TEMP_OFFSET_GET:  // 0xE1 — ответ на запрос смещения
            if (len >= 2) {
                int16_t offset_raw = (int16_t)(payload[0] | (payload[1] << 8));
                float offset = offset_raw / 10.0f;
                ESP_LOGI(TAG, "Arduino temp offset: %.1f C", offset);
                // Можно сохранить в CarData для отладки, но пока не требуется
            }
            break;

        default:
            ESP_LOGD(TAG, "Unhandled Arduino packet: msg_id=0x%02X, len=%d", msg_id, len);
            break;
    }
}