/**
 * @file gps_parser.c
 * @brief Реализация парсера NMEA для GY-GPS6MV2 с ручной/автоматической конфигурацией пинов.
 *
 * Если переданы валидные пины (tx != -1), используется указанная конфигурация.
 * Иначе выполняется автоопределение: перебираются пары {26,27}, {37,38} в обеих ориентациях.
 * gps_alive устанавливается при получении любой NMEA-строки, а не только при фиксации.
 * Последние NMEA-строки сохраняются в кольцевой буфер CarData для отладки.
 */

#include "gps_parser.h"
#include "CarData.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const char *TAG = "GPS_PARSER";

#define GPS_UART_NUM          UART_NUM_0
#define GPS_UART_BUF_SIZE     256
#define NMEA_LINE_MAX         128

static bool gps_alive = false;
static TaskHandle_t gps_task_handle = NULL;

// Пары пинов для автоопределения
static const int auto_pin_pairs[][2] = {
    {26, 27},
    {37, 38}
};
static const int auto_num_pairs = sizeof(auto_pin_pairs) / sizeof(auto_pin_pairs[0]);

static void gps_task(void *arg);
static bool try_gps_config(int tx_pin, int rx_pin, int baud_rate, uint32_t timeout_ms);

static void parse_rmc(const char *line, CarData *data) {
    char buf[80];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    
    char *token = strtok(buf, ",");
    token = strtok(NULL, ","); // время
    if (!token) return;
    token = strtok(NULL, ","); // статус A/V
    if (!token) return;
    
    if (*token == 'A') {
        data->gps_valid = true;
    } else {
        data->gps_valid = false;
        return;
    }
    
    token = strtok(NULL, ",");
    if (!token) return;
    float lat_raw = atof(token);
    int lat_deg = (int)(lat_raw / 100);
    float lat_min = lat_raw - lat_deg * 100;
    data->gps_lat = lat_deg + lat_min / 60.0f;
    
    token = strtok(NULL, ","); // N/S
    if (token && *token == 'S') data->gps_lat = -data->gps_lat;
    
    token = strtok(NULL, ",");
    if (!token) return;
    float lon_raw = atof(token);
    int lon_deg = (int)(lon_raw / 100);
    float lon_min = lon_raw - lon_deg * 100;
    data->gps_lon = lon_deg + lon_min / 60.0f;
    
    token = strtok(NULL, ","); // E/W
    if (token && *token == 'W') data->gps_lon = -data->gps_lon;
    
    token = strtok(NULL, ",");
    if (token) {
        float speed_knots = atof(token);
        data->gps_speed = speed_knots * 1.852f;
    }
}

static void parse_gga(const char *line, CarData *data) {
    char buf[80];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    
    char *token = strtok(buf, ",");
    token = strtok(NULL, ","); // время
    token = strtok(NULL, ","); // широта
    token = strtok(NULL, ","); // N/S
    token = strtok(NULL, ","); // долгота
    token = strtok(NULL, ","); // E/W
    token = strtok(NULL, ","); // качество
    if (token && atoi(token) > 0) {
        data->gps_valid = true;
    }
    token = strtok(NULL, ","); // количество спутников
    if (token) {
        data->gps_satellites = (uint8_t)atoi(token);
    }
}

static void gps_task(void *arg) {
    uint8_t data_buf[GPS_UART_BUF_SIZE];
    char line[NMEA_LINE_MAX];
    int line_pos = 0;
    
    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, data_buf, GPS_UART_BUF_SIZE, pdMS_TO_TICKS(200));
        for (int i = 0; i < len; i++) {
            char c = data_buf[i];
            if (c == '\n' || c == '\r') {
                if (line_pos > 0) {
                    line[line_pos] = '\0';
                    CarData *data = CarData_Get();
                    if (data) {
                        CarData_Lock(10);
                        if (strncmp(line, "$GPRMC", 6) == 0) {
                            gps_alive = true;
                            // Сохраняем в одиночное поле
                            snprintf(data->gps_last_nmea, sizeof(data->gps_last_nmea), "%s", line);
                            // Кольцевой буфер
                            int idx = data->gps_nmea_buf_idx;
                            snprintf(data->gps_nmea_buf[idx], sizeof(data->gps_nmea_buf[idx]), "%s", line);
                            data->gps_nmea_buf_idx = (idx + 1) % GPS_NMEA_BUF_SIZE;
                            if (data->gps_nmea_buf_count < GPS_NMEA_BUF_SIZE) data->gps_nmea_buf_count++;
                            parse_rmc(line, data);
                        } else if (strncmp(line, "$GPGGA", 6) == 0) {
                            gps_alive = true;
                            snprintf(data->gps_last_nmea, sizeof(data->gps_last_nmea), "%s", line);
                            int idx = data->gps_nmea_buf_idx;
                            snprintf(data->gps_nmea_buf[idx], sizeof(data->gps_nmea_buf[idx]), "%s", line);
                            data->gps_nmea_buf_idx = (idx + 1) % GPS_NMEA_BUF_SIZE;
                            if (data->gps_nmea_buf_count < GPS_NMEA_BUF_SIZE) data->gps_nmea_buf_count++;
                            parse_gga(line, data);
                        }
                        CarData_Unlock();
                    }
                    line_pos = 0;
                }
            } else if (line_pos < NMEA_LINE_MAX - 1) {
                line[line_pos++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static bool try_gps_config(int tx_pin, int rx_pin, int baud_rate, uint32_t timeout_ms) {
    uart_driver_delete(GPS_UART_NUM);
    
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    if (uart_driver_install(GPS_UART_NUM, GPS_UART_BUF_SIZE * 2, 0, 0, NULL, 0) != ESP_OK) {
        return false;
    }
    if (uart_param_config(GPS_UART_NUM, &uart_config) != ESP_OK) {
        uart_driver_delete(GPS_UART_NUM);
        return false;
    }
    if (uart_set_pin(GPS_UART_NUM, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        uart_driver_delete(GPS_UART_NUM);
        return false;
    }
    
    ESP_LOGI(TAG, "Trying GPS on TX:%d RX:%d @ %d baud", tx_pin, rx_pin, baud_rate);
    
    uint8_t buf[256];
    char line[NMEA_LINE_MAX];
    int line_pos = 0;
    uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - start < timeout_ms) {
        int len = uart_read_bytes(GPS_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(100));
        for (int i = 0; i < len; i++) {
            char c = buf[i];
            if (c == '\n' || c == '\r') {
                if (line_pos > 0) {
                    line[line_pos] = '\0';
                    if (strncmp(line, "$GPRMC", 6) == 0 || strncmp(line, "$GPGGA", 6) == 0) {
                        ESP_LOGI(TAG, "GPS valid on TX:%d RX:%d", tx_pin, rx_pin);
                        gps_alive = true;
                        return true;
                    }
                    line_pos = 0;
                }
            } else if (line_pos < NMEA_LINE_MAX - 1) {
                line[line_pos++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    uart_driver_delete(GPS_UART_NUM);
    return false;
}

esp_err_t gps_init(int tx_pin, int rx_pin, int baud_rate) {
    if (tx_pin != -1 && rx_pin != -1) {
        if (try_gps_config(tx_pin, rx_pin, baud_rate, 3000)) {
            xTaskCreate(gps_task, "gps_task", 4096, NULL, 1, &gps_task_handle);
            return ESP_OK;
        }
        ESP_LOGW(TAG, "Specified pins failed, falling back to auto-detection");
    }

    for (int i = 0; i < auto_num_pairs; i++) {
        int p1 = auto_pin_pairs[i][0];
        int p2 = auto_pin_pairs[i][1];
        if (try_gps_config(p1, p2, baud_rate, 3000)) {
            xTaskCreate(gps_task, "gps_task", 4096, NULL, 1, &gps_task_handle);
            return ESP_OK;
        }
        if (try_gps_config(p2, p1, baud_rate, 3000)) {
            xTaskCreate(gps_task, "gps_task", 4096, NULL, 1, &gps_task_handle);
            return ESP_OK;
        }
    }
    
    ESP_LOGW(TAG, "GPS not found on any pins");
    return ESP_FAIL;
}
bool gps_is_alive(void) {
    return gps_alive;
}