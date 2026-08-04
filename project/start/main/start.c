#include <stdio.h>

#include "proj_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

static void led_blink_task(void *pvParameters)
{
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(CONFIG_BLINK_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(CONFIG_BLINK_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink", 2048, NULL, 5, NULL);

    // ...
    proj_wifi_init();
    if (proj_wifi_wait_connected(pdMS_TO_TICKS(10000))) {
        ESP_LOGI("start", "WiFi connected");
    } else {
        ESP_LOGE("start", "WiFi connection failed or timed out");
    }    
}