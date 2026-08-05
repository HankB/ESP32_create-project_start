#include <stdio.h>
#include <time.h>

#include "proj_wifi.h"
#include "proj_sntp.h"
#include "proj_mqtt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"

char topic[128];
const char * hostname;

// build a reusable topic. 
// MUST be called after proj_wifi_init() which initializes the hostname
// that proj_wifi_get_hostname() returns.
static const char * build_topic(const char *prefix, const char *location, const char *measurement)
{
    snprintf(topic, sizeof(topic), "%s/%s/%s/%s", prefix, hostname, location, measurement);
    return topic;
}

static void led_blink_task(void *pvParameters)
{
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
    static const TickType_t blink_delay = 250;

    while (1) {
        gpio_set_level(CONFIG_BLINK_GPIO, 1);
        if (proj_sntp_time_synced()) printf("%lld hi at seconds\n", time(0));
        vTaskDelay(pdMS_TO_TICKS(blink_delay));
        gpio_set_level(CONFIG_BLINK_GPIO, 0);
        if (proj_sntp_time_synced()) printf("%lld lo at seconds\n", time(0));
        vTaskDelay(pdMS_TO_TICKS(blink_delay));
    }
}

void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink", 2048, NULL, 5, NULL);
    hostname = generate_hostname();

    proj_wifi_init();
    if (proj_wifi_wait_connected(pdMS_TO_TICKS(10000))) {
        proj_sntp_init();
        if (proj_sntp_wait_synced(pdMS_TO_TICKS(10000))) {
            ESP_LOGI("start", "Time synchronized");
        } else {
            ESP_LOGW("start", "SNTP sync timed out — will keep retrying in background");
        }
        proj_mqtt_init();
        if (proj_mqtt_wait_connected(pdMS_TO_TICKS(10000))) {
            proj_mqtt_publish(build_topic("CS", "lab", "status"), "boot", 0, false);
        }
    }
    // just loop and publish periodically
    build_topic("HA", "lab", "testing");
    static size_t max_payload = 128;
    char    payload[max_payload];
    static const int publish_delay = 10 * 1000;
    while(true) {
        int rssi;
        if( ESP_OK != esp_wifi_sta_get_rssi(&rssi)) rssi=0;
        snprintf(payload, sizeof(payload), 
            "{ \"t\": %lld, \"uptime\":%lld, \"rssi\":%d, \"device\":\"SNTP\" }",
            time(0), esp_timer_get_time()/1000000, rssi);
        proj_mqtt_publish(topic, payload, 0, false);
        vTaskDelay(pdMS_TO_TICKS(publish_delay));
    }
}
