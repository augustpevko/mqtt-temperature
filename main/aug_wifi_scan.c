#include "aug_wifi_scan.h"

#include <string.h>
#include <limits.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event.h>

#define DEFAULT_SCAN_LIST_SIZE CONFIG_SCAN_LIST_SIZE

static const char *TAG = "aug scan";

uint8_t aug_get_least_freq_channel(void)
{
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();//esp_netif_destroy
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));//esp_wifi_deinit
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());//esp_wifi_stop
    esp_wifi_scan_start(NULL, true);//esp_wifi_scan_get_ap_records
    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    uint8_t count[UINT8_MAX + 1] = {0};//we have numbers range less than 256 actually but still
    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);

        count[ap_info[i].primary]++;
    }

    uint8_t min_elem = 0;
    uint8_t min_count = UINT8_MAX;
    for (int i = 1; i < 14; i++) {
        if (i <= 13 && count[i] < min_count) {
            min_elem = i;
            min_count = count[i];
        }
    }

    /*free*/
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }
    ESP_LOGI(TAG, "Not busy channel: %u", min_elem);
    return min_elem;
}