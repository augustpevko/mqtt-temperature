#include "aug_ds18b20.h"

#include <assert.h>
#include <string.h>

#include <esp_event.h>
#include <esp_log.h>

#include "onewire_bus.h"
#include "ds18b20.h"

#include "aug_utility.h"

#define DEFAULT_ONEWIRE_BUS_GPIO CONFIG_ONEWIRE_BUS_GPIO
#define DEFAULT_ONEWIRE_MAX_DS18B20 CONFIG_ONEWIRE_MAX_DS18B20

static const char *TAG = "DS18B20S"; 

static bool is_initialized = false;
static int ds18b20_device_num = 0;
static ds18b20_device_handle_t ds18b20s[DEFAULT_ONEWIRE_MAX_DS18B20];

static esp_err_t initialize_onewire_bus(onewire_bus_handle_t *bus)
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = DEFAULT_ONEWIRE_BUS_GPIO,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };

    AUG_RETURN_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, bus));
    ESP_LOGI(TAG, "1-Wire bus installed on GPIO%d", DEFAULT_ONEWIRE_BUS_GPIO);
    return ESP_OK;
}

static esp_err_t search_ds18b20_devices(onewire_bus_handle_t bus)
{
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;

    AUG_RETURN_CHECK(onewire_new_device_iter(bus, &iter));
    ESP_LOGI(TAG, "Device iterator created, start searching...");

    do {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK) {
            ds18b20_config_t ds_cfg = {};
            if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &ds18b20s[ds18b20_device_num]) == ESP_OK) {
                ESP_LOGI(TAG, "Found a DS18B20[%d], address: %016llX", ds18b20_device_num, next_onewire_device.address);
                ds18b20_device_num++;
                if (ds18b20_device_num >= DEFAULT_ONEWIRE_MAX_DS18B20) {
                    ESP_LOGI(TAG, "Max DS18B20 number reached, stop searching...");
                    break;
                }
            } else {
                ESP_LOGI(TAG, "Found an unknown device, address: %016llX", next_onewire_device.address);
            }
        }
    } while (search_result != ESP_ERR_NOT_FOUND);

    AUG_RETURN_CHECK(onewire_del_device_iter(iter));
    ESP_LOGI(TAG, "Searching done, %d DS18B20 device(s) found", ds18b20_device_num);
    if (ds18b20_device_num <= 0)
        return ESP_FAIL;
    return ESP_OK;
}

static esp_err_t set_resolution_for_devices()
{
    for (int i = 0; i < ds18b20_device_num; i++) {
        AUG_RETURN_CHECK(ds18b20_set_resolution(ds18b20s[i], DS18B20_RESOLUTION_12B));
    }
    return ESP_OK;
}

esp_err_t aug_ds18b20_init()
{
    ESP_LOGI(TAG, "initializing DS18B20");
    ds18b20_device_num = 0;
    onewire_bus_handle_t bus;

    // Initialize 1-Wire bus and DS18B20 devices
    AUG_RETURN_CHECK(initialize_onewire_bus(&bus));
    AUG_RETURN_CHECK(search_ds18b20_devices(bus));
    AUG_RETURN_CHECK(set_resolution_for_devices());
    is_initialized = true;
    
    return ESP_OK;
}

esp_err_t aug_ds18b20_deinit()
{
    assert(is_initialized && "ds18b20 is not initialized");
    ESP_LOGI(TAG, "deinitializing DS18B20");
    ds18b20_device_num = 0;
    memset(ds18b20s, 0, sizeof(ds18b20s));
    is_initialized = false;
    return ESP_OK;
}

size_t aug_get_sensors_number()
{
    assert(is_initialized && "ds18b20 is not initialized");
    return ds18b20_device_num;
}

float aug_get_temperature(size_t index)
{
    assert(is_initialized && "ds18b20 is not initialized");
    float temperature = 0;
    ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(ds18b20s[index]));
    ESP_ERROR_CHECK(ds18b20_get_temperature(ds18b20s[index], &temperature));
    return temperature;
}