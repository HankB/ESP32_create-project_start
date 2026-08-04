#include <stdio.h>
#include <time.h>

#include "proj_wifi.h"
#include "proj_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

static void led_blink_task(void *pvParameters)
{
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
    static const TickType_t blink_delay = 1000;

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

    // ...
    proj_wifi_init();
    if (proj_wifi_wait_connected(pdMS_TO_TICKS(10000))) {
        proj_sntp_init();
        if (proj_sntp_wait_synced(pdMS_TO_TICKS(10000))) {
            ESP_LOGI("start", "Time synchronized");
        } else {
            ESP_LOGW("start", "SNTP sync timed out — will keep retrying in background");
        }
    }
}
