idf_component_register(
    SRCS "fiona_phrase_loader.c"
    INCLUDE_DIRS "."
    REQUIRES espressif__cjson fatfs
)