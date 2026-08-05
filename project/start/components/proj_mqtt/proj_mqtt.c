#include "proj_mqtt.h"

#include <stdio.h>
#include <string.h>

#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "proj_mqtt";

static EventGroupHandle_t mqtt_event_group = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static char hostname[16];   /* "esp32-" + 6 hex chars + nul */

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

static void generate_hostname(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(hostname, sizeof(hostname), "esp32-%02x%02x%02x",
             mac[3], mac[4], mac[5]);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    (void) handler_args;
    (void) base;
    (void) event_data;

    switch ((esp_mqtt_event_id_t) event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to broker");
        xEventGroupSetBits(mqtt_event_group, PROJ_MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected from broker");
        xEventGroupClearBits(mqtt_event_group, PROJ_MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error event");
        break;
    default:
        break;
    }
}

esp_err_t proj_mqtt_init(void)
{
    esp_err_t err;

    err = ensure_ok_or_already_done(esp_netif_init(), "esp_netif_init");
    if (err != ESP_OK) {
        return err;
    }

    err = ensure_ok_or_already_done(esp_event_loop_create_default(),
                                     "esp_event_loop_create_default");
    if (err != ESP_OK) {
        return err;
    }

    mqtt_event_group = xEventGroupCreate();
    if (mqtt_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    generate_hostname();
    ESP_LOGI(TAG, "MQTT client ID / topic prefix: %s", hostname);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_PROJ_MQTT_BROKER_URI,
        .credentials.client_id = hostname,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return ESP_FAIL;
    }

    err = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                          mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_register_event failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_start failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "proj_mqtt_init() complete, connection in progress (broker: %s)",
             CONFIG_PROJ_MQTT_BROKER_URI);
    return ESP_OK;
}

EventGroupHandle_t proj_mqtt_get_event_group(void)
{
    return mqtt_event_group;
}

bool proj_mqtt_wait_connected(TickType_t timeout_ticks)
{
    if (mqtt_event_group == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(mqtt_event_group,
                                            PROJ_MQTT_CONNECTED_BIT,
                                            pdFALSE, pdFALSE, timeout_ticks);
    return (bits & PROJ_MQTT_CONNECTED_BIT) != 0;
}

const char *proj_mqtt_get_hostname(void)
{
    return hostname;
}

int proj_mqtt_publish(const char *location, const char *measurement,
                       const char *payload, int qos, bool retain)
{
    if (mqtt_client == NULL) {
        ESP_LOGW(TAG, "proj_mqtt_publish() called before proj_mqtt_init()");
        return -1;
    }

    char topic[128];
    snprintf(topic, sizeof(topic), "HA/%s/%s/%s", hostname, location, measurement);

    /* len=0 tells esp-mqtt to use strlen(payload) internally. */
    return esp_mqtt_client_publish(mqtt_client, topic, payload, 0,
                                    qos, retain ? 1 : 0);
}
