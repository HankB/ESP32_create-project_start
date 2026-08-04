#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bit set on the SNTP event group once time has been synchronized at least
 * once. There is no "fail" bit — unlike WiFi, SNTP has no give-up condition;
 * it keeps retrying against the configured servers indefinitely. */
#define PROJ_SNTP_TIME_SET_BIT   BIT0

/*
 * Starts the SNTP client using CONFIG_PROJ_SNTP_SERVER_PRIMARY (if set) and
 * CONFIG_PROJ_SNTP_SERVER_BACKUP. Must be called after WiFi is connected —
 * this function does NOT call esp_netif_init() or
 * esp_event_loop_create_default(), since proj_wifi_init() already does.
 *
 * Time sync happens asynchronously in the background. Returns ESP_OK once
 * the SNTP client has been started, not once time is actually synced —
 * use proj_sntp_wait_synced() or the event group bit above for that.
 */
esp_err_t proj_sntp_init(void);

/*
 * Returns the event group backing PROJ_SNTP_TIME_SET_BIT, so callers can
 * wait on or inspect sync state directly. Valid only after
 * proj_sntp_init() has been called.
 */
EventGroupHandle_t proj_sntp_get_event_group(void);

/*
 * Blocking wait for the first successful time sync, up to timeout_ticks.
 * Returns true if synced within the timeout, false on timeout.
 */
bool proj_sntp_wait_synced(TickType_t timeout_ticks);

/*
 * Convenience non-blocking status check. Equivalent to
 * proj_sntp_wait_synced(0) - returns immediately with current state.
 */
static inline bool proj_sntp_time_synced(void)
{
    return proj_sntp_wait_synced(0);
}

#ifdef __cplusplus
}
#endif