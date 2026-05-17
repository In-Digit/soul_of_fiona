#include "uart_rx_imu.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "esp_log.h"

static const char *TAG = "UART_IMU";

// Промежуточные значения для сбора полного вектора ускорения
static int16_t pending_ax = 0;
static int16_t pending_ay = 0;
static bool has_ax_ay = false;

void uart_rx_imu_process(uint8_t msg_id, const uint8_t *payload, uint8_t len) {
    CarData *data = CarData_Get();
    if (!data) return;

    CarData_Lock(10);

    switch (msg_id) {
        case MSG_REQ_ACCEL:   // 0xA0 – первые два компонента (ax, ay)
            if (len >= 4) {
                pending_ax = (int16_t)(payload[0] | (payload[1] << 8));
                pending_ay = (int16_t)(payload[2] | (payload[3] << 8));
                has_ax_ay = true;
            }
            break;

        case MSG_ACCEL_Z:     // 0xA5 – третий компонент (az) и завершение
            if (len >= 4 && has_ax_ay) {
                int16_t az = (int16_t)(payload[0] | (payload[1] << 8));
                data->accel_x = pending_ax / 100.0f;
                data->accel_y = pending_ay / 100.0f;
                data->accel_z = az / 100.0f;
                data->accelDirty = true;
                has_ax_ay = false;
            }
            break;

        case MSG_REQ_GYRO:    // 0xA1 – ответ шлюза
            if (len >= 6) {
                int16_t gx = (int16_t)(payload[0] | (payload[1] << 8));
                int16_t gy = (int16_t)(payload[2] | (payload[3] << 8));
                int16_t gz = (int16_t)(payload[4] | (payload[5] << 8));
                data->gyro_x = gx / 100.0f;
                data->gyro_y = gy / 100.0f;
                data->gyro_z = gz / 100.0f;
                data->gyroDirty = true;
            }
            break;

        case MSG_REQ_TILT:    // 0xA2 – ответ шлюза
            if (len >= 2) {
                data->tilt_roll  = (int8_t)payload[0];
                data->tilt_pitch = (int8_t)payload[1];
                data->tiltDirty = true;
            }
            break;

        case MSG_REQ_CALIB_STATUS:   // 0xA4 – ответ шлюза
            if (len >= 1) {
                data->calib_status = payload[0];
                ESP_LOGI(TAG, "Calib status: 0x%02X", payload[0]);
            }
            break;

        default:
            break;
    }

    CarData_Unlock();
}