idf_component_register(
    SRCS "proj_ds18b20.c"
    INCLUDE_DIRS "include"
    REQUIRES "onewire_bus" "ds18b20"
)
