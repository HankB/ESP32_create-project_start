idf_component_register(
    SRCS "start.c"
    REQUIRES ""
    PRIV_REQUIRES "proj_wifi" "esp_driver_gpio" "proj_sntp" "proj_mqtt" "esp_timer" "proj_ds18b20" "proj_ota"
    )