#include <stdio.h>

#include "test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

void app_main(void)
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