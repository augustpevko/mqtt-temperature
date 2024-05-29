#include "aug_wifi_ap.h"

#include <string.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include "nvs_flash.h"

#include "aug_wifi_scan.h"
#include "aug_utility.h"

ESP_EVENT_DEFINE_BASE(AUG_WIFI_AP_EVENTS);

static const char *TAG = "aug wifi ap mode";

static aug_wifi_ap_config_t ap_config = {};
static esp_netif_t* ap_netif = NULL;
static esp_event_loop_handle_t* event_loop_handle = NULL;
static int current_connections = 0;
TaskHandle_t idle_check_task_handle = NULL;

static esp_event_handler_instance_t instance_any_id;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        ++current_connections;
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
        --current_connections;
    }
}

static void idle_check_task(void* params)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(DEFAULT_TIMEOUT_IDLE * 1000));
        if (!current_connections) {
            ESP_ERROR_CHECK(esp_event_post_to(*event_loop_handle, AUG_WIFI_AP_EVENTS, 
                AUG_WIFI_AP_EVENT_IDLE, NULL, 0, portMAX_DELAY));
        }
    }
}

esp_err_t aug_wifi_ap_start(esp_event_loop_handle_t* _event_loop_handle)
{
    ESP_LOGI(TAG, "initializing wifi-AP resources");
    event_loop_handle = _event_loop_handle;
    current_connections = 0;
    idle_check_task_handle = NULL;
    
#if !defined(CONFIG_AP_MANUAL_CHANNEL)
    ap_config.wifi_config.ap.channel = aug_get_least_freq_channel();
#endif
    ap_netif = esp_netif_create_default_wifi_ap();//esp_netif_destroy
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    AUG_RETURN_CHECK(esp_wifi_init(&cfg));//esp_wifi_deinit

    AUG_RETURN_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));//esp_event_handler_instance_unregister

    AUG_RETURN_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));//esp_wifi_restore
    AUG_RETURN_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config.wifi_config));//esp_wifi_restore
    AUG_RETURN_CHECK(esp_wifi_start());//esp_wifi_stop

    ESP_LOGI(TAG, "access point initialization finished. SSID:%s password:%s channel:%d",
             ap_config.wifi_config.ap.ssid, ap_config.wifi_config.ap.password, 
             ap_config.wifi_config.ap.channel);
    
    if (xTaskCreate(idle_check_task, "idle_check_task", 
            1024 * 1, NULL, 5, &idle_check_task_handle) != pdPASS) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t aug_wifi_ap_stop(void)
{
    ESP_LOGI(TAG, "deinitializing wifi-AP resources");
    current_connections = 0;
    vTaskDelete(idle_check_task_handle);
    idle_check_task_handle = NULL;
    AUG_RETURN_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    AUG_RETURN_CHECK(esp_wifi_stop());
    AUG_RETURN_CHECK(esp_wifi_restore());
    AUG_RETURN_CHECK(esp_wifi_deinit());
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }
    return ESP_OK;
}

bool aug_wifi_ap_is_init(void)
{
    if (ap_netif)
        return true;
    return false;
}

aug_wifi_ap_config_t* aug_wifi_ap_get_config(void)
{
    return &ap_config;
}
