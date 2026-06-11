idf_component_register(
    SRCS "rtc.c"
    INCLUDE_DIRS "."
    REQUIRES driver espressif__esp32_p4_function_ev_board
)