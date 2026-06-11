/**
 * @file rtc.c
 * @brief Реализация драйвера RTC RX8025.
 *
 * Использует I2C-шину, предоставляемую BSP платы.
 * Поддерживает чтение и запись полей времени в BCD-формате.
 */

#include "rx8025_rtc.h"
#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>

#define I2C_MASTER_NUM            BSP_I2C_NUM
#define RX8025_I2C_ADDRESS        0x32
#define RX8025_TIME_REGISTER_ADDR 0x00

static const char *TAG = "RTC_DRV";
static i2c_master_dev_handle_t rtc_dev_handle = NULL;

// Функция для преобразования BCD в число
static int bcd_to_int(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Функция для преобразования числа в BCD
static uint8_t int_to_bcd(int num) {
    return (((num / 10) << 4) | (num % 10));
}

// Основная функция инициализации RTC
esp_err_t rtc_init(void) {
    esp_err_t ret = ESP_OK;
    if (rtc_dev_handle) {
        ESP_LOGW(TAG, "RTC already initialized");
        return ESP_OK;
    }

    // 1. Получаем хэндл I2C шины от BSP платы.
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
    if (i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus handle. I2C initialization skipped.");
        return ESP_ERR_INVALID_STATE;
    }

    // 2. Настраиваем конфигурацию устройства RX8025 на шине.
    i2c_device_config_t rtc_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RX8025_I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };

    // 3. Добавляем устройство на шину, получая его уникальный хэндл.
    ret = i2c_master_bus_add_device(i2c_bus_handle, &rtc_dev_cfg, &rtc_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add RTC device to I2C bus (0x%x)", ret);
        return ret;
    }

    ESP_LOGI(TAG, "RTC successfully initialized");
    return ESP_OK;
}

// Функция для чтения времени
esp_err_t rtc_get_time(rtc_time_t *time) {
    if (!rtc_dev_handle) return ESP_ERR_INVALID_STATE;
    if (!time) return ESP_ERR_INVALID_ARG;
    memset(time, 0, sizeof(rtc_time_t));

    uint8_t reg_addr = RX8025_TIME_REGISTER_ADDR;
    uint8_t data[7] = {0};

    esp_err_t ret = i2c_master_transmit_receive(rtc_dev_handle, &reg_addr, 1, data, 7, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time from RTC (0x%x)", ret);
        return ret;
    }

    // Преобразуем считанные BCD-значения в обычные числа.
    time->sec  = bcd_to_int(data[0] & 0x7F);
    time->min  = bcd_to_int(data[1] & 0x7F);
    time->hour = bcd_to_int(data[2] & 0x3F);
    time->day  = bcd_to_int(data[4] & 0x3F);
    time->mon  = bcd_to_int(data[5] & 0x1F);
    time->year = bcd_to_int(data[6]) + 2000;

    ESP_LOGI(TAG, "RTC time read: %04d-%02d-%02d %02d:%02d:%02d",
             time->year, time->mon, time->day, time->hour, time->min, time->sec);
    return ESP_OK;
}

// Функция для установки времени
esp_err_t rtc_set_time(const rtc_time_t *time) {
    if (!rtc_dev_handle) return ESP_ERR_INVALID_STATE;
    if (!time) return ESP_ERR_INVALID_ARG;

    uint8_t data[8];
    data[0] = RX8025_TIME_REGISTER_ADDR;
    data[1] = int_to_bcd(time->sec);
    data[2] = int_to_bcd(time->min);
    data[3] = int_to_bcd(time->hour);
    data[4] = 0x01; // day of week, можно игнорировать
    data[5] = int_to_bcd(time->day);
    data[6] = int_to_bcd(time->mon);
    data[7] = int_to_bcd(time->year - 2000);

    esp_err_t ret = i2c_master_transmit(rtc_dev_handle, data, sizeof(data), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set RTC time (0x%x)", ret);
        return ret;
    }

    ESP_LOGI(TAG, "RTC time set: %04d-%02d-%02d %02d:%02d:%02d",
             time->year, time->mon, time->day, time->hour, time->min, time->sec);
    return ESP_OK;
}