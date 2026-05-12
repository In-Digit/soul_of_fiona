idf_component_register(
    SRCS "system_actions.c"
    INCLUDE_DIRS "."
    REQUIRES car_data uart_protocol lvgl__lvgl
)