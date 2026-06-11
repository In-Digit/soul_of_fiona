/**
 * @file wifi_manager.c
 * @brief Минимальная инициализация Wi‑Fi станции через ESP‑Hosted.
 *
 * Подключается один раз до старта LVGL, используя жёстко заданные сети.
 * Не содержит фоновых задач, мьютексов или сохранения учётных данных.
 */

#include "wifi_manager.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

static const char *TAG = "WIFI_MANAGER";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static char s_ip_address[16] = "0.0.0.0";

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi‑Fi disconnected, trying to reconnect...");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", s_ip_address);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

const char* wifi_manager_get_ip(void) {
    return s_ip_address;
}

static esp_err_t wifi_manager_init(void)
{
    static bool initialized = false;
    if (initialized) return ESP_OK;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    initialized = true;
    return ESP_OK;
}

static esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
    if (!ssid || !password) return ESP_ERR_INVALID_ARG;

    wifi_manager_init();

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    snprintf((char*)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", ssid);
    snprintf((char*)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to %s...", ssid);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to %s", ssid);
        return ESP_OK;
    }
    ESP_LOGW(TAG, "Failed to connect to %s", ssid);
    return ESP_FAIL;
}

esp_err_t wifi_manager_connect_best(void)
{
    // 1. Primary (машина)
    if (wifi_manager_connect("Tri-Al(m)", "Ford-Fiona") == ESP_OK) {
        return ESP_OK;
    }

    // 2. Secondary (дом)
    if (wifi_manager_connect("Tri-AL", "Aq1Sw2De3Fr4") == ESP_OK) {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Could not connect to any Wi‑Fi network");
    return ESP_FAIL;
}