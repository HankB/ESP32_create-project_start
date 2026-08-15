#include <stdio.h>
#include <time.h>

#include "proj_wifi.h"
#include "proj_sntp.h"
#include "proj_mqtt.h"
#include "proj_ota.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "proj_ds18b20.h"
#include "math.h"

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

/*
 * ESP32 WROOM is active high
 * ESP32-C3 mini is active low
 */
#define LED_ACTIVE_LOW    0  // Set to 0 if active high

#if LED_ACTIVE_LOW
#define LED_ON    0  
#define LED_OFF   1
#else
  #define LED_ON    1   
  #define LED_OFF   0
#endif

static void blink(uint count)
{
    while(count--) {
        gpio_set_level(CONFIG_BLINK_GPIO, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(CONFIG_BLINK_GPIO, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}
static void led_blink_task(void *pvParameters)
{
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
    static const TickType_t blink_delay = 1000;

    while (1) {
        if( proj_mqtt_connected())
            blink(3);
        else if(proj_wifi_connected())
            blink(2);
        else
            blink(1);

        vTaskDelay(pdMS_TO_TICKS(blink_delay));        
    }
}

#include "proj_ds18b20.h"
#include "math.h"

static void ds18b20_task(void *arg)
{
    proj_ds18b20_init();
    int n = proj_ds18b20_get_device_count();
    float temps[CONFIG_PROJ_DS18B20_MAX_DEVICES];
    static const size_t topic_len=96;
    char topics[CONFIG_PROJ_DS18B20_MAX_DEVICES][topic_len];

    /*
     * Topics for my use are required to be distinct for each sensor so
     * they will include the sensor ID to achieve that.
     */
    for(int i=0; i<n; i++) {
        snprintf(topics[i], topic_len, "HA/%s/%s/temperature_",
                 generate_hostname(), "roaming");
        proj_ds18b20_get_address_string(i, topics[i]+strlen(topics[i]), topic_len - strlen(topics[i]) );
    }

    while (1) {
        if (n > 0) {
            proj_ds18b20_read_all(temps, n);
            for (int i = 0; i < n; i++) {
                if (!isnan(temps[i])) {
                    char payload[64];
                    /* "location" here is a placeholder - see note below */
                    snprintf(payload, sizeof(payload), "{\"t\":%lld, \"temp\":%.2f, \"device\":\"DS18B20\"}",
                        time(0), temps[i]*9.0/5.0 + 32.0);
                    proj_mqtt_publish(topics[i], payload, 0, false);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink", 2048, NULL, 5, NULL);
    xTaskCreate(ds18b20_task, "read_ds18b20", 2048, NULL, 5, NULL);

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

    // ... after WiFi and MQTT connect:
    if (proj_wifi_wait_connected(pdMS_TO_TICKS(10000)) &&
        proj_mqtt_wait_connected(pdMS_TO_TICKS(10000))) {
        proj_ota_confirm_boot_ok();   /* only meaningful/reached if both succeeded */
        proj_ota_init();
    }
    
    proj_ds18b20_init();
        
    // just loop and publish periodically
    build_topic("HA", "lab", "testing");
    static size_t max_payload = 256;
    char    payload[max_payload];
    static const int publish_delay = 10 * 1000;
    while(true) {
        int rssi;
        if( ESP_OK != esp_wifi_sta_get_rssi(&rssi)) rssi=0;
        snprintf(payload, sizeof(payload), 
            "{ \"t\": %lld, \"uptime\":%lld, \"rssi\":%d, \"device\":\"ESP32\"," 
            " \"mqtt_stats\":[%ld, %ld, %ld, %ld], \"wifi_stats\":[%lu, %lu, %u]}",
            time(0), esp_timer_get_time()/1000000, rssi,
            proj_mqtt_get_connect_count(),
            proj_mqtt_get_disconnect_count(),
            proj_mqtt_get_publish_success_count(),
            proj_mqtt_get_publish_fail_count(),
            proj_wifi_get_connect_count(),
            proj_wifi_get_disconnect_count(),
            (uint)proj_wifi_get_last_disconnect_reason()
            
        );
        proj_mqtt_publish(topic, payload, 0, false);
        vTaskDelay(pdMS_TO_TICKS(publish_delay));
    }
}
