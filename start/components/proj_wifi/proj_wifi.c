#include "proj_wifi.h"
#include "secrets.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_mac.h"

static const char *TAG = "proj_wifi";

static EventGroupHandle_t wifi_event_group = NULL;
static char hostname[16] = "";   /* "esp32-" + 6 hex chars + nul */

#define WIFI_RETRY_BASE_DELAY_MS   1000
#define WIFI_RETRY_MAX_DELAY_MS    60000
#define WIFI_RETRY_SHIFT_CAP       6

static esp_timer_handle_t reconnect_timer;
static int retry_shift = 0;

static uint32_t wifi_connect_count = 0;
static uint32_t wifi_disconnect_count = 0;
static uint8_t  last_disconnect_reason = 0;

const char *generate_hostname(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(hostname, sizeof(hostname), "esp32-%02x%02x%02x",
             mac[3], mac[4], mac[5]);
    return hostname;
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

static void reconnect_timer_cb(void *arg)
{
    ESP_LOGI(TAG, "Attempting WiFi reconnect");
    esp_wifi_connect();
}

static void schedule_reconnect(void)
{
    uint32_t delay_ms = WIFI_RETRY_BASE_DELAY_MS << retry_shift;
    if (delay_ms > WIFI_RETRY_MAX_DELAY_MS) {
        delay_ms = WIFI_RETRY_MAX_DELAY_MS;
    }
    if (retry_shift < WIFI_RETRY_SHIFT_CAP) {
        retry_shift++;
    }
    ESP_LOGW(TAG, "Reconnecting in %lu ms", (unsigned long) delay_ms);
    esp_timer_stop(reconnect_timer);   /* harmless if not currently running */
    esp_timer_start_once(reconnect_timer, (uint64_t) delay_ms * 1000);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *) event_data;
        last_disconnect_reason = disconn->reason;
        wifi_disconnect_count++;
        xEventGroupClearBits(wifi_event_group, PROJ_WIFI_CONNECTED_BIT);
        ESP_LOGW(TAG, "WiFi disconnected, reason=%d", disconn->reason);
        schedule_reconnect();

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_shift = 0;
        wifi_connect_count++;
        xEventGroupSetBits(wifi_event_group, PROJ_WIFI_CONNECTED_BIT);
    }
}

esp_err_t proj_wifi_init(void)
{
    esp_err_t err;

    /* NVS is required by the WiFi driver for calibration/config storage. */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    generate_hostname();   /* idempotent - fine if already called earlier */
    ESP_LOGI(TAG, "Hostname / MQTT client ID: %s", hostname);

    err = ensure_ok_or_already_done(esp_netif_init(), "esp_netif_init");
    if (err != ESP_OK) {
        return err;
    }

    err = ensure_ok_or_already_done(esp_event_loop_create_default(),
                                     "esp_event_loop_create_default");
    if (err != ESP_OK) {
        return err;
    }

    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &reconnect_timer_cb,
        .name = "wifi_reconnect",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &reconnect_timer));

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "proj_wifi_init() complete, connection in progress");
    return ESP_OK;
}

EventGroupHandle_t proj_wifi_get_event_group(void)
{
    return wifi_event_group;
}

bool proj_wifi_wait_connected(TickType_t timeout_ticks)
{
    if (wifi_event_group == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                            PROJ_WIFI_CONNECTED_BIT,
                                            pdFALSE, pdFALSE, timeout_ticks);
    return (bits & PROJ_WIFI_CONNECTED_BIT) != 0;
}

const char *proj_wifi_get_hostname(void)
{
    if (hostname[0] == '\0') {
        return "unknown";
    }
    return hostname;
}

uint32_t proj_wifi_get_connect_count(void)
{
    return wifi_connect_count;
}

uint32_t proj_wifi_get_disconnect_count(void)
{
    return wifi_disconnect_count;
}

uint8_t proj_wifi_get_last_disconnect_reason(void)
{
    return last_disconnect_reason;
}

bool proj_wifi_get_rssi(int8_t *rssi_out)
{
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) != ESP_OK) {
        return false;
    }
    *rssi_out = info.rssi;
    return true;
}