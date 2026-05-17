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

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = 480 * 800,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = { .buff_dma = false, .buff_spiram = true, .sw_rotate = true }
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    bsp_display_lock(0);
    ui_init();
    bsp_display_unlock();

    // Инициализация UART
    #define NUM_UART_PORTS 2
    static uart_pin_config_t scan_configs[NUM_UART_PORTS][2] = {
        //{{ .tx = 37, .rx = 38 }, { .tx = 38, .rx = 37 }},
        {{ .tx = 46, .rx = 47 }, { .tx = 47, .rx = 46 }},
        {{ .tx = 26, .rx = 27 }, { .tx = 27, .rx = 26 }}
    };
    static int scan_idx[NUM_UART_PORTS] = {0, 0};
    uart_protocol_deinit();
    uart_pin_config_t try_pins[NUM_UART_PORTS];
    for (int i = 0; i < NUM_UART_PORTS; i++) {
        try_pins[i] = scan_configs[i][scan_idx[i]];
    }
    uart_protocol_init(try_pins, NUM_UART_PORTS);
    uart_send_broadcast(MSG_WHO_IS_HERE, NULL, 0);

    fiona_core_init();

    ESP_LOGI(TAG, "Fiona core started");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}