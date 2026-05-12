idf_component_register(
    SRCS "config_manager.c"
    INCLUDE_DIRS "."
    REQUIRES car_data fatfs espressif__cjson
)