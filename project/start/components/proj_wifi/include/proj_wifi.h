#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bits set on the WiFi event group, exposed so callers (e.g. the LED task)
 * can xEventGroupWaitBits() on connection state without polling. */
#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1

/*
 * Initializes NVS (if not already done), the netif/event loop, and starts
 * WiFi in station mode using credentials from secrets.h. Connection happens
 * asynchronously — this function returns once the connection attempt has
 * been kicked off, not once it succeeds.
 *
 * Returns ESP_OK if WiFi init/start succeeded, or an error from the
 * underlying esp_wifi_*/esp_netif_* calls. Does NOT mean "connected" —
 * use wifi_wait_connected() or the event group bits above for that.
 */
esp_err_t init_wifi(void);

/*
 * Returns the event group backing WIFI_CONNECTED_BIT / WIFI_FAIL_BIT, so
 * callers can wait on or inspect connection state directly. Valid only
 * after init_wifi() has been called.
 */
EventGroupHandle_t wifi_get_event_group(void);

/*
 * Convenience blocking wait for connection, up to timeout_ticks.
 * Returns true if WIFI_CONNECTED_BIT was set within the timeout,
 * false on timeout or if WIFI_FAIL_BIT was set (retries exhausted).
 */
bool wifi_wait_connected(TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif
