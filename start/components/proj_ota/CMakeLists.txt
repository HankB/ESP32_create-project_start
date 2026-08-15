idf_component_register(
    SRCS "proj_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES "app_update" "esp_https_ota" "proj_wifi" "proj_mqtt"
)
