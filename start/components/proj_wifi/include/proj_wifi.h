#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bits set on the WiFi event group, exposed so callers (e.g. the LED task)
 * can xEventGroupWaitBits() on connection state without polling. */
#define PROJ_WIFI_CONNECTED_BIT   BIT0
#define PROJ_WIFI_FAIL_BIT        BIT1

/*
 * Initializes NVS (if not already done), the netif/event loop, and starts
 * WiFi in station mode using credentials from secrets.h. Connection happens
 * asynchronously - this function returns once the connection attempt has
 * been kicked off, not once it succeeds.
 *
 * Returns ESP_OK if WiFi init/start succeeded, or an error from the
 * underlying esp_wifi_ and esp_netif_ calls. Does NOT mean "connected" -
 * use proj_wifi_wait_connected() or the event group bits above for that.
 */
esp_err_t proj_wifi_init(void);

/*
 * Returns the event group backing PROJ_WIFI_CONNECTED_BIT / PROJ_WIFI_FAIL_BIT,
 * so callers can wait on or inspect connection state directly. Valid only
 * after proj_wifi_init() has been called.
 */
EventGroupHandle_t proj_wifi_get_event_group(void);

/*
 * Blocking wait for connection, up to timeout_ticks.
 * Returns true if PROJ_WIFI_CONNECTED_BIT was set within the timeout,
 * false on timeout or if PROJ_WIFI_FAIL_BIT was set (retries exhausted).
 */
bool proj_wifi_wait_connected(TickType_t timeout_ticks);

/* 
 * Generate a hostname in the form "esp32-nnnnnn" where nnnnnn is the hex
 * representation if the last three bytes of the MAC address.
 * This does not depend on WiFi initialization and can be called at
 * any time.
 */
const char * generate_hostname(void);

/*
 * Convenience non-blocking status check. Equivalent to
 * proj_wifi_wait_connected(0) - returns immediately with current state.
 */
static inline bool proj_wifi_connected(void)
{
    return proj_wifi_wait_connected(0);
}

/*
 * Returns this device's generated hostname/client ID ("esp32-XXXXXX"), as
 * used for both the MQTT client ID and the topic prefix. Valid only after
 * proj_mqtt_init() has been called.
 */
const char *proj_wifi_get_hostname(void);

#ifdef __cplusplus
}
#endif