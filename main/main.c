/**
 * @file main.c
 * @brief Главный файл приложения.
 *
 * Инициализирует NVS, SPIFFS, SD-карту, дисплей, аудиокодек, UART,
 * подключается к Wi‑Fi (до GUI), запускает ядро Фионы.
 * При наличии сохранённых настроек UART использует их.
 */

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "ui.h"
#include "CarData.h"
#include "uart_protocol.h"
#include "sd_utils.h"
#include "fiona_core.h"
#include "wifi_manager.h"
#include "ntp_sync.h"
#include "gps_parser.h"

static const char *TAG = "FIONA_MAIN";

void app_main(void)
{
    setenv("TZ", "MSK-3", 1);
    tzset();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (!CarData_Init()) {
        ESP_LOGE(TAG, "Failed to initialize CarData");
        return;
    }

    ret = bsp_spiffs_mount();
    if (ret != ESP_OK) ESP_LOGW(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "SPIFFS mounted");

    ret = bsp_sdcard_mount();
    if (ret != ESP_OK) ESP_LOGW(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "SD card mounted");

    ret = bsp_extra_codec_init();
    if (ret != ESP_OK) ESP_LOGW(TAG, "Audio codec init failed: %s", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "Audio codec initialized");

    // Wi‑Fi до GUI
    esp_err_t wifi_err = wifi_manager_connect_best();
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "Wi‑Fi connection failed, continuing without network");
    } else {
        // Обновляем статус Wi‑Fi в CarData
        CarData *data = CarData_Get();
        if (data) {
            CarData_Lock(10);
            data->wifiConnected = true;
            CarData_Unlock();
        }
        // Автоматическая синхронизация времени при успешном подключении Wi‑Fi
        ntp_sync_request();
    }

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = 1024 * 600,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = { .buff_dma = false, .buff_spiram = true, .sw_rotate = true }
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    bsp_display_lock(0);
    ui_init();
    bsp_display_unlock();

    // --------------- Инициализация UART (шлюз + Arduino) ---------------
    CarData *data = CarData_Get();
    int gw_tx = 46, gw_rx = 47, gw_baud = 115200;//921600;
    int ard_tx = 32, ard_rx = 33, ard_baud = 115200;

    // Если есть проверенные настройки для шлюза, используем их
    if (data && data->gw_configured) {
        gw_tx = data->gw_tx_pin;
        gw_rx = data->gw_rx_pin;
        gw_baud = data->gw_baud_rate;
        ESP_LOGI(TAG, "Using saved gateway UART config: TX=%d, RX=%d, baud=%d", gw_tx, gw_rx, gw_baud);
    }
    // Если есть проверенные настройки для Arduino, используем их
    if (data && data->arduino_configured) {
        ard_tx = data->arduino_tx_pin;
        ard_rx = data->arduino_rx_pin;
        ard_baud = data->arduino_baud_rate;
        ESP_LOGI(TAG, "Using saved Arduino UART config: TX=%d, RX=%d, baud=%d", ard_tx, ard_rx, ard_baud);
    }

    uart_pin_config_t uart_pins[2] = {
        { .tx = gw_tx, .rx = gw_rx },
        { .tx = ard_tx, .rx = ard_rx }
    };

    uart_protocol_deinit();
    uart_protocol_init(uart_pins, 2, gw_baud, ard_baud);
    uart_send_broadcast(MSG_WHO_IS_HERE, NULL, 0);

    // --------------- Инициализация GPS ---------------
    // Если есть проверенные настройки, передаём их; иначе используем автоопределение (передаём -1)
    int gps_tx = -1, gps_rx = -1, gps_baud = 9600;
    if (data && data->gps_configured) {
        gps_tx = data->gps_tx_pin;
        gps_rx = data->gps_rx_pin;
        gps_baud = data->gps_baud_rate;
        ESP_LOGI(TAG, "Using saved GPS config: TX=%d, RX=%d, baud=%d", gps_tx, gps_rx, gps_baud);
    }
    esp_err_t gps_ret = gps_init(gps_tx, gps_rx, gps_baud);
    if (gps_ret != ESP_OK) {
        ESP_LOGW(TAG, "GPS initialization failed, continuing without GPS");
    }

    fiona_core_init();
    ESP_LOGI(TAG, "Fiona core started");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}