#pragma once
/*
 * Starts the SNTP client using CONFIG_PROJ_SNTP_SERVER_PRIMARY (if set) and
 * CONFIG_PROJ_SNTP_SERVER_BACKUP.
 *
 * This function ensures esp_netif_init() and esp_event_loop_create_default()
 * have been called, calling them itself if needed - so it's safe to call
 * proj_sntp_init() even if no other component (e.g. proj_wifi) has already
 * brought up netif/event-loop infrastructure. If you swap out proj_wifi for
 * a different network transport (Ethernet, etc.), proj_sntp_init() does not
 * assume WiFi specifically - only that *some* IP-capable interface exists
 * and is connected, which remains the caller's responsibility to arrange
 * before calling this function.
 *
 * Time sync happens asynchronously in the background. Returns ESP_OK once
 * the SNTP client has been started, not once time is actually synced -
 * use proj_sntp_wait_synced() or the event group bit above for that.
 */
esp_err_t proj_sntp_init(void);
