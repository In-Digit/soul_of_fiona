idf_component_register(
    SRCS "fiona_core.c"
         "fiona_background.c"
         "fiona_observers.c"
         "fiona_animations.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash spiffs car_data lvgl__lvgl uart_protocol sd_utils fiona_soul fiona_brain config_manager esp_timer driver esp_adc bsp_extra espressif__esp32_p4_function_ev_board
)