idf_component_register(
    SRCS "ntp_sync.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_netif rtc_driver
)