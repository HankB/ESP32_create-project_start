idf_component_register(
    SRCS "proj_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_wifi" "esp_netif" "esp_event" "nvs_flash"
)
