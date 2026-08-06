#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initializes the 1-Wire bus on CONFIG_PROJ_DS18B20_GPIO (RMT-based, not
 * software bit-banged - not vulnerable to FreeRTOS scheduling jitter the
 * way a delay-loop bit-bang implementation would be) and enumerates all
 * DS18B20 devices found, up to CONFIG_PROJ_DS18B20_MAX_DEVICES.
 *
 * Returns ESP_OK if at least one DS18B20 was found, ESP_ERR_NOT_FOUND if
 * the bus initialized but no DS18B20 devices were detected, or another
 * error if the bus itself failed (e.g. GPIO conflict).
 */
esp_err_t proj_ds18b20_init(void);

/* Number of DS18B20 devices found during proj_ds18b20_init(). */
int proj_ds18b20_get_device_count(void);

/*
 * Writes the 64-bit 1-Wire ROM address of device `index` (0-based, in
 * discovery order) as a 16-char uppercase hex string into buf (needs at
 * least 17 bytes). Useful for mapping a specific physical sensor to a
 * topic/location if more than one is on the bus. Returns false if index
 * is out of range.
 */
bool proj_ds18b20_get_address_string(int index, char *buf, size_t buf_len);

/*
 * Triggers a simultaneous temperature conversion on all discovered
 * devices, then reads each one, writing up to max_count results into
 * temps_out. temps_out[i] corresponds to the same device index used by
 * proj_ds18b20_get_address_string(). A failed individual read is written
 * as NAN rather than shifting later indices, so index alignment always
 * holds - check each value with isnan() if you need to detect failures.
 *
 * This call BLOCKS for the conversion duration - up to ~750ms at the
 * default 12-bit resolution. Call it from a dedicated sensor task, not a
 * latency-sensitive one.
 *
 * Returns ESP_OK if the trigger itself succeeded (individual reads may
 * still be NAN), or an error if the trigger failed or no devices were
 * found.
 */
esp_err_t proj_ds18b20_read_all(float *temps_out, int max_count);

#ifdef __cplusplus
}
#endif
