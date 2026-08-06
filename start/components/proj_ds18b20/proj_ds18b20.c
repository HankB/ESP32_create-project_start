#include "proj_ds18b20.h"

#include <math.h>
#include <stdio.h>

#include "esp_log.h"
#include "onewire_bus.h"
//#include "onewire_bus_rmt.h"
#include "ds18b20.h"

static const char *TAG = "proj_ds18b20";

static onewire_bus_handle_t bus = NULL;
static ds18b20_device_handle_t devices[CONFIG_PROJ_DS18B20_MAX_DEVICES];
static onewire_device_address_t addresses[CONFIG_PROJ_DS18B20_MAX_DEVICES];
static int device_count = 0;

esp_err_t proj_ds18b20_init(void)
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = CONFIG_PROJ_DS18B20_GPIO,
        .flags = {
            .en_pull_up = true,
        },
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,   /* 1 ROM command + 8 ROM number + 1 device command */
    };

    esp_err_t err = onewire_new_bus_rmt(&bus_config, &rmt_config, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "onewire_new_bus_rmt failed: %s", esp_err_to_name(err));
        return err;
    }

    onewire_device_iter_handle_t iter = NULL;
    err = onewire_new_device_iter(bus, &iter);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "onewire_new_device_iter failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Searching 1-Wire bus on GPIO %d...", CONFIG_PROJ_DS18B20_GPIO);
    device_count = 0;

    onewire_device_t next_device;
    esp_err_t search_result;
    do {
        search_result = onewire_device_iter_get_next(iter, &next_device);
        if (search_result == ESP_OK) {
            if (device_count >= CONFIG_PROJ_DS18B20_MAX_DEVICES) {
                ESP_LOGW(TAG, "More devices found than PROJ_DS18B20_MAX_DEVICES (%d), ignoring extras",
                         CONFIG_PROJ_DS18B20_MAX_DEVICES);
                break;
            }
            ds18b20_config_t ds_cfg = {};
            if (ds18b20_new_device_from_enumeration(&next_device, &ds_cfg,
                                                     &devices[device_count]) == ESP_OK) {
                ds18b20_get_device_address(devices[device_count], &addresses[device_count]);
                ESP_LOGI(TAG, "Found DS18B20[%d], address: %016llX",
                         device_count, (unsigned long long) addresses[device_count]);
                device_count++;
            } else {
                ESP_LOGI(TAG, "Found non-DS18B20 1-Wire device, address: %016llX",
                         (unsigned long long) next_device.address);
            }
        }
    } while (search_result != ESP_ERR_NOT_FOUND);

    onewire_del_device_iter(iter);
    ESP_LOGI(TAG, "1-Wire search complete, %d DS18B20 device(s) found", device_count);

    if (device_count == 0) {
        ESP_LOGW(TAG, "No DS18B20 devices found on the bus");
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

int proj_ds18b20_get_device_count(void)
{
    return device_count;
}

bool proj_ds18b20_get_address_string(int index, char *buf, size_t buf_len)
{
    if (index < 0 || index >= device_count || buf == NULL) {
        return false;
    }
    snprintf(buf, buf_len, "%016llX", (unsigned long long) addresses[index]);
    return true;
}

esp_err_t proj_ds18b20_read_all(float *temps_out, int max_count)
{
    if (device_count == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = ds18b20_trigger_temperature_conversion_for_all(bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "trigger_temperature_conversion_for_all failed: %s", esp_err_to_name(err));
        return err;
    }

    int n = device_count < max_count ? device_count : max_count;
    for (int i = 0; i < n; i++) {
        float t;
        esp_err_t read_err = ds18b20_get_temperature(devices[i], &t);
        if (read_err == ESP_OK) {
            temps_out[i] = t;
        } else {
            ESP_LOGW(TAG, "Failed to read DS18B20[%d]: %s", i, esp_err_to_name(read_err));
            temps_out[i] = NAN;
        }
    }
    return ESP_OK;
}
