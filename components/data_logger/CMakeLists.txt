idf_component_register(SRCS "data_logger.c" "wifi.c"
INCLUDE_DIRS "include"
REQUIRES mqtt nvs_flash
)