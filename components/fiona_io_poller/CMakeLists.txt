idf_component_register(
    SRCS "fiona_io_poller.c"
    INCLUDE_DIRS "."
    REQUIRES freertos car_data uart_protocol
)