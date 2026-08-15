#include "proj_ota.h"
#include "proj_wifi.h"
#include "proj_mqtt.h"

#include <stdio.h>
#include <string.h>

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_log.h"

static const char *TAG = "proj_ota";

esp_err_t proj_ota_confirm_boot_ok(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) != ESP_OK) {
        /* CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE likely not set - nothing to do. */
        return ESP_OK;
    }

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        /* Caller is expected to have already confirmed WiFi + MQTT are up
         * before calling this - that IS the diagnostic. If we got here,
         * both worked, so confirm the image. */
        ESP_LOGI(TAG, "New firmware confirmed healthy - canceling rollback");
        esp_ota_mark_app_valid_cancel_rollback();
    }
    return ESP_OK;
}

static void ota_update_task(void *pvParameters)
{
    char *url = (char *) pvParameters;
    ESP_LOGI(TAG, "Starting OTA update from %s", url);

    esp_http_client_config_t http_config = {
        .url = url,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_err_t err = esp_https_ota(&ota_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA succeeded - rebooting into new firmware");
        free(url);
        esp_restart();
        /* unreachable */
    }

    ESP_LOGE(TAG, "OTA failed: %s - staying on current firmware", esp_err_to_name(err));
    free(url);
    vTaskDelete(NULL);
}

static void ota_command_handler(const char *topic, const char *data, int data_len)
{
    (void) topic;
    if (data_len <= 0) {
        ESP_LOGW(TAG, "Empty OTA command payload, ignoring");
        return;
    }

    char *url = strdup(data);   /* freed inside ota_update_task */
    if (url == NULL) {
        ESP_LOGE(TAG, "strdup failed for OTA URL");
        return;
    }

    /* Larger stack: TLS/HTTP + flash write need meaningful headroom. */
    BaseType_t ok = xTaskCreate(ota_update_task, "ota_update", 8192, url, 5, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OTA task");
        free(url);
    }
}

esp_err_t proj_ota_init(void)
{
    char topic[64];
    snprintf(topic, sizeof(topic), "HA/%s/system/ota", generate_hostname());

    esp_err_t err = proj_mqtt_subscribe(topic, 1, ota_command_handler);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Subscribed to OTA command topic: %s", topic);
    }
    return err;
}
