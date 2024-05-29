#include "aug_wifi_sta.h"

#include <string.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include "aug_utility.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
ESP_EVENT_DEFINE_BASE(AUG_WIFI_STA_EVENTS);

static const char* TAG = "aug wifi sta mode";

static aug_wifi_sta_config_t sta_config = {};
static esp_netif_t* sta_netif = NULL;

static esp_event_loop_handle_t* event_loop_handle = NULL;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

static int retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Trying to connect to the AP...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < sta_config.max_retry) {
            ESP_LOGI(TAG, "Retrying to connect to the AP...");
            esp_wifi_connect();
            retry_num++;
        } 
        else {
            ESP_ERROR_CHECK(esp_event_post_to(*event_loop_handle, AUG_WIFI_STA_EVENTS, 
                AUG_WIFI_STA_EVENT_FAILED_ATTEMPTS, NULL, 0, portMAX_DELAY));
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            retry_num = 0;
        }
        ESP_LOGI(TAG,"Connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t aug_wifi_sta_connect(esp_event_loop_handle_t* _event_loop_handle)
{
    ESP_LOGI(TAG, "Initializing wifi-STA resources");
    event_loop_handle = _event_loop_handle;
    wifi_config_t* wifi_config = &sta_config.wifi_config;

    sta_netif = esp_netif_create_default_wifi_sta();//esp_netif_destroy
    
    //i assume init config depends on netif(?) 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    AUG_RETURN_CHECK(esp_wifi_init(&cfg));//esp_wifi_deinit

    s_wifi_event_group = xEventGroupCreate();//vEventGroupDelete
    AUG_RETURN_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    AUG_RETURN_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    AUG_RETURN_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));//esp_wifi_restore
    AUG_RETURN_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_config));//esp_wifi_restore
    AUG_RETURN_CHECK(esp_wifi_start());//esp_wifi_stop
    
    ESP_LOGI(TAG, "Waiting for connection");
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to ap SSID:%s password:%s",
                 wifi_config->sta.ssid, wifi_config->sta.password);
        return ESP_OK;
    } 
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config->sta.ssid, wifi_config->sta.password);
        return ESP_FAIL;
    } 
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }
}

esp_err_t aug_wifi_sta_disconnect(void)
{
    ESP_LOGI(TAG, "Deinitializing wifi-STA resources");
    event_loop_handle = NULL;
    AUG_RETURN_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    AUG_RETURN_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    AUG_RETURN_CHECK(esp_wifi_restore());
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    AUG_RETURN_CHECK(esp_wifi_deinit());
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }
    return ESP_OK;
}

bool aug_wifi_sta_is_init(void)
{
    if (sta_netif)
        return true;
    return false;
}

aug_wifi_sta_config_t* aug_wifi_sta_get_config(void)
{
    return &sta_config;
}