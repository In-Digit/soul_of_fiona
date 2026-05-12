idf_component_register(
    SRCS main.c
    INCLUDE_DIRS .
    REQUIRES nvs_flash spiffs car_data espressif__esp32_p4_function_ev_board bsp_extra gui lvgl__lvgl uart_protocol esp_timer
)