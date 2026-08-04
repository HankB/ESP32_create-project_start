#include "proj_sntp.h"

#include <string.h>
#include <time.h>

#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "proj_sntp";

static EventGroupHandle_t sntp_event_group = NULL;

static void time_sync_notification_cb(struct timeval *tv)
{
    time_t now = tv->tv_sec;
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", &tm_utc);
    ESP_LOGI(TAG, "Time synchronized: %s", buf);

    xEventGroupSetBits(sntp_event_group, PROJ_SNTP_TIME_SET_BIT);
}

/* Returns ESP_OK if the call succeeded OR if the underlying subsystem was
 * already initialized (ESP_ERR_INVALID_STATE). Any other error is real. */
static esp_err_t ensure_ok_or_already_done(esp_err_t err, const char *what)
{
    if (err == ESP_OK) {
        return ESP_OK;
    }
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGD(TAG, "%s already initialized, continuing", what);
        return ESP_OK;
    }
    ESP_LOGE(TAG, "%s failed: %s", what, esp_err_to_name(err));
    return err;
}

esp_err_t proj_sntp_init(void)
{
    esp_err_t err;

    /* Ensure netif/event-loop infrastructure exists, regardless of whether
     * some other component (proj_wifi or otherwise) already set it up. */
    err = ensure_ok_or_already_done(esp_netif_init(), "esp_netif_init");
    if (err != ESP_OK) {
        return err;
    }

    err = ensure_ok_or_already_done(esp_event_loop_create_default(),
                                     "esp_event_loop_create_default");
    if (err != ESP_OK) {
        return err;
    }

    sntp_event_group = xEventGroupCreate();
    if (sntp_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_PROJ_SNTP_SERVER_BACKUP);
    config.sync_cb = time_sync_notification_cb;

    if (strlen(CONFIG_PROJ_SNTP_SERVER_PRIMARY) > 0) {
        config.num_of_servers = 2;
        config.servers[0] = CONFIG_PROJ_SNTP_SERVER_PRIMARY;
        config.servers[1] = CONFIG_PROJ_SNTP_SERVER_BACKUP;
        ESP_LOGI(TAG, "SNTP servers: %s (primary), %s (backup)",
                 CONFIG_PROJ_SNTP_SERVER_PRIMARY, CONFIG_PROJ_SNTP_SERVER_BACKUP);
    } else {
        ESP_LOGI(TAG, "SNTP server: %s (no primary configured)",
                 CONFIG_PROJ_SNTP_SERVER_BACKUP);
    }

    err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_sntp_init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "proj_sntp_init() complete, sync in progress");
    return ESP_OK;
}

EventGroupHandle_t proj_sntp_get_event_group(void)
{
    return sntp_event_group;
}

bool proj_sntp_wait_synced(TickType_t timeout_ticks)
{
    EventBits_t bits = xEventGroupWaitBits(sntp_event_group,
                                            PROJ_SNTP_TIME_SET_BIT,
                                            pdFALSE, pdFALSE, timeout_ticks);
    return (bits & PROJ_SNTP_TIME_SET_BIT) != 0;
}