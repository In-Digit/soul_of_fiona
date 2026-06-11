/**
 * @file fiona_io_poller.c
 * @brief Реализация фоновой задачи опроса устройств.
 *
 * Задача в бесконечном цикле последовательно запрашивает данные
 * у шлюза и Arduino, с задержкой 50 мс между запросами, чтобы
 * не перегружать шину и дать время на приём ответа.
 */

#include "fiona_io_poller.h"
#include "uart_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Приоритет задачи (ниже, чем у рендеринга)
#define IO_POLLER_PRIORITY   2
#define IO_POLLER_STACK_SIZE 2048

// Периоды опроса (в тиках FreeRTOS, 1 тик = 10 мс при 100 Гц)
#define FAST_POLL_TICKS      pdMS_TO_TICKS(100)   // 10 раз в секунду
#define SLOW_POLL_TICKS      pdMS_TO_TICKS(1000)  // 1 раз в секунду
#define REQUEST_DELAY_TICKS  pdMS_TO_TICKS(50)    // задержка между запросами

static void io_poller_task(void *arg) {
    // Счётчики для распределения запросов
    uint8_t fast_cycle = 0;
    uint8_t slow_cycle = 0;

    while (1) {
        // === БЫСТРЫЕ ЗАПРОСЫ (каждые ~100 мс) ===
        if (uart_is_gateway_alive()) {
            switch (fast_cycle) {
                case 0: uart_send_to_gateway(MSG_SPEED, NULL, 0); break;
                case 1: uart_send_to_gateway(MSG_RPM, NULL, 0); break;
                case 2: uart_send_to_gateway(MSG_THROTTLE, NULL, 0); break;
                case 3: uart_send_to_gateway(MSG_INST_FUEL, NULL, 0); break;
                case 4: uart_send_to_gateway(MSG_REQ_ACCEL, NULL, 0); break;
                // Дополнительные быстрые параметры можно добавить здесь
            }
            fast_cycle = (fast_cycle + 1) % 5;
            vTaskDelay(REQUEST_DELAY_TICKS);  // пауза для приёма ответа
        }

        // === МЕДЛЕННЫЕ ЗАПРОСЫ (каждые ~1 с) ===
        if (uart_is_gateway_alive()) {
            switch (slow_cycle) {
                case 0: uart_send_to_gateway(MSG_COOLANT_TEMP, NULL, 0); break;
                case 1: uart_send_to_gateway(MSG_VOLTAGE, NULL, 0); break;
                case 2: uart_send_to_gateway(MSG_FUEL_LEVEL, NULL, 0); break;
                case 3: uart_send_to_gateway(MSG_ODO, NULL, 0); break;
                case 4: uart_send_to_gateway(MSG_RANGE, NULL, 0); break;
                case 5: uart_send_to_gateway(MSG_TRIP_TIME, NULL, 0); break;
                case 6: uart_send_to_gateway(MSG_TRIP_PAUSE, NULL, 0); break;
                case 7: uart_send_to_gateway(MSG_TRIP_FUEL, NULL, 0); break;
                case 8: uart_send_to_gateway(MSG_TRIP_DIST, NULL, 0); break;
                case 9: uart_send_to_gateway(MSG_TRIP_STATUS, NULL, 0); break;
                case 10: uart_send_to_gateway(MSG_TRIP_COST, NULL, 0); break;
                case 11: uart_send_to_gateway(MSG_CLIMATE_TELEMETRY, NULL, 0); break;
                // и т.д.
            }
            slow_cycle = (slow_cycle + 1) % 12;
            vTaskDelay(REQUEST_DELAY_TICKS);
        }

        // === ARDUINO (запрос телеметрии) ===
        if (uart_is_arduino_alive()) {
            uart_send_to_arduino(MSG_FAN_TELEMETRY, NULL, 0);
            vTaskDelay(REQUEST_DELAY_TICKS);
        }

        // Основная пауза перед следующей итерацией быстрого цикла
        vTaskDelay(pdMS_TO_TICKS(10));  // ~100 Гц, но с учётом задержек будет реже
    }
}

void fiona_io_poller_start(void) {
    xTaskCreate(io_poller_task, "io_poller", IO_POLLER_STACK_SIZE, NULL, IO_POLLER_PRIORITY, NULL);
}