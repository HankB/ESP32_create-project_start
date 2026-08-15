#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Call once, early in start.c, after WiFi and MQTT are confirmed connected
 * (within a reasonable timeout of your choosing). If the running partition
 * is pending verification (i.e. this boot followed an OTA update), this
 * confirms the app is healthy - proving WiFi + MQTT both work - and calls
 * esp_ota_mark_app_valid_cancel_rollback(). If WiFi/MQTT connection wasn't
 * already confirmed by the caller, do NOT call this - see note in .c file.
 *
 * If CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is not set, this is a no-op
 * that always returns ESP_OK (there's nothing to confirm).
 */
esp_err_t proj_ota_confirm_boot_ok(void);

/*
 * Subscribes to this device's OTA command topic
 * (HA/<hostname>/system/ota) via proj_mqtt. The expected payload is a
 * plain HTTP URL pointing at a firmware .bin. On receipt, triggers a
 * background OTA download-and-flash; on success, reboots automatically.
 * On failure, logs the error and leaves the currently running firmware
 * untouched - no reboot, no partition switch.
 *
 * Must be called after proj_mqtt_init().
 */
esp_err_t proj_ota_init(void);

#ifdef __cplusplus
}
#endif
