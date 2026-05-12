idf_component_register(
    SRCS "uart_protocol.c"
         "uart_rx_task.c"
         "uart_rx_dashboard.c"
         "uart_rx_stats.c"
         "uart_rx_arduino.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer car_data
)