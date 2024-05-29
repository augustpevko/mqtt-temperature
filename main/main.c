#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>

#include "aug_nvs.h"
#include "aug_utility.h"
#include "aug_wifi.h"
#include "aug_wifi_sta.h"
#include "aug_wifi_ap.h"
#include "aug_http_server.h"
#include "aug_mqtt_client.h"
#include "aug_ds18b20.h"

static const char *TAG = "main";

static uint8_t get_mac_hash() 
{
    uint8_t mac[6];
    esp_err_t ret;

    ret = esp_base_mac_addr_get(mac);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to get MAC address");
        return 0;
    }

    uint8_t hash = 0;
    for (int i = 0; i < sizeof(mac); i++) {
        hash = hash * hash + mac[i];
    }
    return hash;
}

static void publish_task(void* params)
{
    (void)params;
    uint8_t mac_hash = get_mac_hash();
    float temperature;
    size_t sensors_number = aug_get_sensors_number();
    char topic[128] = {};
    char temperature_str[8] = {};
    
    while (1) {
        for (size_t i = 0; i < sensors_number && aug_mqtt_is_connected(); ++i) {
            temperature = aug_get_temperature(i);
            memset(topic, 0, sizeof(topic));
            snprintf(topic, sizeof(topic), 
                "/devices/rtl-esp-wroom%i[%i]/controls/temperature/meta/type", mac_hash, i + 1);
            aug_mqtt_publish_str(topic, "temperature");
            
            memset(topic, 0, sizeof(topic));
            snprintf(topic, sizeof(topic), 
                "/devices/rtl-esp-wroom%i[%i]/controls/temperature/meta/readonly", mac_hash, i + 1);
            aug_mqtt_publish_str(topic, "1");
            
            memset(topic, 0, sizeof(topic));
            snprintf(topic, sizeof(topic), 
                "/devices/rtl-esp-wroom%i[%i]/controls/temperature", mac_hash, i + 1);
            memset(temperature_str, 0, sizeof(temperature_str));
            snprintf(temperature_str, sizeof(temperature_str), "%.2f", temperature);
            aug_mqtt_publish_str(topic, temperature_str);
        }
        vTaskDelay(pdMS_TO_TICKS(DEFAULT_PUBLISH_RATE * 1000));
    }
}

static void deinit_modules()
{
    if (aug_http_is_init())
        ESP_ERROR_CHECK(aug_http_stop_webserver());
    if (aug_wifi_sta_is_init())
        ESP_ERROR_CHECK(aug_wifi_sta_disconnect());
    if (aug_wifi_ap_is_init())
        ESP_ERROR_CHECK(aug_wifi_ap_stop());
}

static void write_nvs_data()
{
    ESP_ERROR_CHECK(aug_nvs_set_ap_config());
    ESP_ERROR_CHECK(aug_nvs_set_sta_config());
    ESP_ERROR_CHECK(aug_nvs_set_mqtt_config());
}

static void callback_init_sta(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)base;
    (void)id;
    (void)event_data;
    esp_event_loop_handle_t* event_loop_handle = (esp_event_loop_handle_t*)handler_arg;
    if (aug_mqtt_is_connected())
        ESP_ERROR_CHECK(aug_mqtt_stop());
    deinit_modules();

    if (aug_wifi_sta_connect(event_loop_handle) != ESP_OK)
        return;
    aug_http_start(event_loop_handle);
    ESP_ERROR_CHECK(aug_mqtt_start());
}

static void callback_init_ap(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)base;
    (void)id;
    (void)event_data;
    esp_event_loop_handle_t* event_loop_handle = (esp_event_loop_handle_t*)handler_arg;
    if (aug_mqtt_is_connected())
        ESP_ERROR_CHECK(aug_mqtt_stop());
    deinit_modules();

    ESP_ERROR_CHECK(aug_wifi_ap_start(event_loop_handle));
    aug_http_start(event_loop_handle);
}

static void callback_init_mqtt(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)handler_arg;
    (void)base;
    (void)id;
    (void)event_data;
    ESP_ERROR_CHECK(aug_mqtt_set_uri());
}

static void callback_esp_restart(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)handler_arg;
    (void)base;
    (void)id;
    (void)event_data;
    if (aug_mqtt_is_connected())
        ESP_ERROR_CHECK(aug_mqtt_stop());
    deinit_modules();
    write_nvs_data();
    ESP_LOGI(TAG, "Rebooting...");

    esp_restart();
}

esp_event_loop_handle_t event_loop_init()
{
    esp_event_loop_handle_t event_loop_handle;
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 2,
        .task_name = "second_loop",
        .task_priority = configMAX_PRIORITIES - 1,
        .task_stack_size = configMINIMAL_STACK_SIZE * 4,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &event_loop_handle));
    return event_loop_handle;
}

void register_events(esp_event_loop_handle_t* event_loop_handle)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*event_loop_handle, AUG_WIFI_STA_EVENTS, 
        AUG_WIFI_STA_EVENT_FAILED_ATTEMPTS, callback_init_ap, event_loop_handle, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*event_loop_handle, AUG_WIFI_AP_EVENTS, 
        AUG_WIFI_AP_EVENT_IDLE, callback_init_sta, event_loop_handle, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
        AUG_HTTP_SERVER_EVENT_INIT_STA, callback_init_sta, event_loop_handle, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
        AUG_HTTP_SERVER_EVENT_INIT_MQTT, callback_init_mqtt, event_loop_handle, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
        AUG_HTTP_SERVER_EVENT_OTA_UPDATE, callback_esp_restart, event_loop_handle, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
        AUG_HTTP_SERVER_EVENT_RESTART, callback_esp_restart, event_loop_handle, NULL));
}

static void main_init(esp_event_loop_handle_t* event_loop_handle)
{
    *event_loop_handle = event_loop_init();
    esp_err_t is_sta_config_found = aug_nvs_get_sta_config();
    if (is_sta_config_found != ESP_OK) {
        ESP_LOGI(TAG, "No station mode configuration found in the NVS");
        aug_wifi_sta_config_t* sta_config = aug_wifi_sta_get_config();
        memset(sta_config, 0, sizeof(*sta_config));
#if defined(CONFIG_WIFI_INFO)
        *sta_config = aug_wifi_get_default_sta_config();
#endif
    }
    esp_err_t is_ap_config_found = aug_nvs_get_ap_config();
    if (is_ap_config_found != ESP_OK) {
        ESP_LOGI(TAG, "No access point mode configuration found in the NVS");
        aug_wifi_ap_config_t* ap_config = aug_wifi_ap_get_config();
        memset(ap_config, 0, sizeof(*ap_config));
        *ap_config = aug_wifi_get_default_ap_config();
    }
    if (is_sta_config_found == ESP_OK) {
        aug_wifi_sta_connect(event_loop_handle);
    }
    else {
#if defined(CONFIG_WIFI_INFO)
    aug_wifi_sta_connect(event_loop_handle);
#else
    ESP_ERROR_CHECK(aug_wifi_ap_start(event_loop_handle));
#endif
    }

    if (aug_nvs_get_mqtt_config() != ESP_OK) {
        ESP_LOGI(TAG, "No MQTT client configuration found in the NVS");
        aug_mqtt_set_default_uri();
    }
    aug_mqtt_uri_t mqtt_uri = aug_mqtt_get_uri();
    esp_mqtt_client_config_t* mqtt_config = aug_mqtt_get_config();
    mqtt_config->broker.address.uri = mqtt_uri.uri_str;

    register_events(event_loop_handle);
    ESP_ERROR_CHECK(aug_ds18b20_init());
    aug_http_start(event_loop_handle);
    ESP_ERROR_CHECK(aug_mqtt_init());
    if (aug_wifi_sta_is_init())
        ESP_ERROR_CHECK(aug_mqtt_start());
    xTaskCreate(publish_task, "publish_task", 1024 * 3, NULL, 5, NULL);
}

static void main_loop(void)
{
    esp_event_loop_handle_t event_loop_handle;
    main_init(&event_loop_handle);
    
    bool shutdown = false;
    while (!shutdown)
        vTaskDelay(5000);
    //some free work here
}

static void aug_nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void verify_app(void)
{
    const esp_partition_t* partition = esp_ota_get_running_partition();
	esp_ota_img_states_t ota_state;

	if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK) {
		if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
			esp_ota_mark_app_valid_cancel_rollback();
		}
	}
}

void app_main(void)
{
    aug_nvs_init();
    verify_app();
    ESP_ERROR_CHECK(aug_wifi_init());

    main_loop();
}