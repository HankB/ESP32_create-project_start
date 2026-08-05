#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Unlike PROJ_WIFI_CONNECTED_BIT / PROJ_SNTP_TIME_SET_BIT, this bit is NOT
 * latched - it reflects current connection state, and is cleared on
 * disconnect. The mqtt-client library reconnects automatically in the
 * background, so "currently connected" is the useful signal here. */
#define PROJ_MQTT_CONNECTED_BIT   BIT0

/*
 * Starts the MQTT client using CONFIG_PROJ_MQTT_BROKER_URI, connecting
 * anonymously (no TLS, no username/password - intended for use only
 * within a trusted, isolated IoT VLAN). Client ID and topic prefix are
 * both derived at runtime from the station MAC address as "esp32-XXXXXX"
 * (last 3 MAC bytes in hex), matching the existing Pi hostname convention.
 *
 * Must be called once WiFi is connected (the client won't reach the
 * broker before then, though calling this before WiFi is up won't crash -
 * it'll just sit retrying). Like proj_sntp_init(), this function ensures
 * esp_netif_init() / esp_event_loop_create_default() have run, so it does
 * not strictly require proj_wifi to be the one that set them up.
 *
 * Connection happens asynchronously. Returns ESP_OK once the client has
 * been started, not once it's actually connected - use
 * proj_mqtt_wait_connected() or the event group bit for that.
 */
esp_err_t proj_mqtt_init(void);

EventGroupHandle_t proj_mqtt_get_event_group(void);

/* Blocking wait for current connection, up to timeout_ticks. */
bool proj_mqtt_wait_connected(TickType_t timeout_ticks);

static inline bool proj_mqtt_connected(void)
{
    return proj_mqtt_wait_connected(0);
}

/*
 * Publishes payload to topic "HA/<hostname>/<location>/<measurement>",
 * matching the existing Pi topic/payload convention. qos must be 0, 1,
 * or 2; retain follows normal MQTT retained-message semantics.
 *
 * Returns the library's message ID (>=0) on success, or -1 if the
 * publish could not be queued (e.g. proj_mqtt_init() not yet called).
 * A non-negative return does NOT guarantee delivery for QoS 0.
 */
int proj_mqtt_publish(const char *location, const char *measurement,
                       const char *payload, int qos, bool retain);

#ifdef __cplusplus
}
#endif
