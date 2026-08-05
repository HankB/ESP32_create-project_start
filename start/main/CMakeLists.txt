idf_component_register(
    SRCS "start.c"
    REQUIRES ""
    PRIV_REQUIRES "proj_wifi" "esp_driver_gpio" "proj_sntp" "proj_mqtt"
    )