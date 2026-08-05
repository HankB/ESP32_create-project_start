idf_component_register(
    SRCS "proj_mqtt.c"
    INCLUDE_DIRS "include"
    REQUIRES "mqtt" "esp_netif" "esp_event" "proj_wifi"
)
