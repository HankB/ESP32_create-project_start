#include "proj_mqtt.h"
#include "proj_wifi.h"

#include <stdio.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "proj_mqtt";

static EventGroupHandle_t mqtt_event_group = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static char status_topic[64];

static uint32_t mqtt_connect_count = 0;
static uint32_t mqtt_disconnect_count = 0;
static uint32_t mqtt_publish_success_count = 0;
static uint32_t mqtt_publish_fail_count = 0;

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

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    (void) handler_args;
    (void) base;
    (void) event_data;

    switch ((esp_mqtt_event_id_t) event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to broker");
        mqtt_connect_count++;
        xEventGroupSetBits(mqtt_event_group, PROJ_MQTT_CONNECTED_BIT);
        esp_mqtt_client_publish(mqtt_client, status_topic, "online",
                                 strlen("online"), 1, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected from broker");
        mqtt_disconnect_count++;
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

    /* generate_hostname() is idempotent - safe to call again here even if
     * proj_wifi_init() (or start.c directly) already called it. */
    const char *host = generate_hostname();
    snprintf(status_topic, sizeof(status_topic), "CM/%s/system/status", host);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_PROJ_MQTT_BROKER_URI,
        .credentials.client_id = host,
        .session.last_will.topic = status_topic,
        .session.last_will.msg = "offline",
        .session.last_will.msg_len = strlen("offline"),
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
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

int proj_mqtt_publish(const char *topic, const char *payload, int qos, bool retain)
{
    if (mqtt_client == NULL) {
        ESP_LOGW(TAG, "proj_mqtt_publish() called before proj_mqtt_init()");
        mqtt_publish_fail_count++;
        return -1;
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0,
                                          qos, retain ? 1 : 0);
    if (msg_id < 0) {
        mqtt_publish_fail_count++;
    } else {
        mqtt_publish_success_count++;
    }
    return msg_id;
}

uint32_t proj_mqtt_get_connect_count(void)
{
    return mqtt_connect_count;
}

uint32_t proj_mqtt_get_disconnect_count(void)
{
    return mqtt_disconnect_count;
}

uint32_t proj_mqtt_get_publish_success_count(void)
{
    return mqtt_publish_success_count;
}

uint32_t proj_mqtt_get_publish_fail_count(void)
{
    return mqtt_publish_fail_count;
}