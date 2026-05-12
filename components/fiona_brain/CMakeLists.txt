idf_component_register(
    SRCS "fiona_brain.c"
    INCLUDE_DIRS "."
    REQUIRES car_data nvs_flash espressif__cjson fiona_phrase_loader
)