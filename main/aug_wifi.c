#include "aug_wifi.h"
#include <stdint.h>
#include <string.h>

#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>

#include "aug_utility.h"

//for sta mode
#if defined(CONFIG_WIFI_INFO)

#define DEFAULT_STA_WIFI_SSID       CONFIG_STA_WIFI_SSID
#define DEFAULT_STA_WIFI_PASSWORD   CONFIG_STA_WIFI_PASSWORD
#define DEFAULT_MAXIMUM_RETRY       CONFIG_MAXIMUM_RETRY

//sae mode and h2e identifier 
#if defined(CONFIG_WPA3_SAE_PWE_HUNT_AND_PECK)
#define DEFAULT_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define DEFAULT_H2E_IDENTIFIER ""
#elif CONFIG_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define DEFAULT_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define DEFAULT_H2E_IDENTIFIER CONFIG_WIFI_PW_ID
#elif CONFIG_WPA3_SAE_PWE_BOTH
#define DEFAULT_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define DEFAULT_H2E_IDENTIFIER CONFIG_WIFI_PW_ID
#endif//CONFIG_WPA3_SAE_PWE_HUNT_AND_PECK
//scan threshold
#if defined(CONFIG_WIFI_AUTH_OPEN)
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_WIFI_AUTH_WEP
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_WIFI_AUTH_WPA_PSK
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_WIFI_AUTH_WPA2_PSK
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_WIFI_AUTH_WPA_WPA2_PSK
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_WIFI_AUTH_WPA3_PSK
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_WIFI_AUTH_WPA2_WPA3_PSK
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_WIFI_AUTH_WAPI_PSK
#define DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif//CONFIG_WIFI_AUTH_OPEN
#endif//CONFIG_WIFI_INFO

//for ap mode
#define DEFAULT_AP_WIFI_SSID        CONFIG_AP_WIFI_SSID
#define DEFAULT_AP_WIFI_PASSWORD    CONFIG_AP_WIFI_PASSWORD

#if defined(CONFIG_AP_MANUAL_CHANNEL)
#define DEFAULT_AP_WIFI_CHANNEL     CONFIG_AP_WIFI_CHANNEL
#endif//AP_MANUAL_CHANNEL
#define DEFAULT_MAX_STA_CONN        CONFIG_MAX_STA_CONN
#define DEFAULT_TIMEOUT_IDLE        CONFIG_TIMEOUT_IDLE

static const char *TAG = "aug wifi";

aug_wifi_ap_config_t aug_wifi_get_default_ap_config(void)
{
    aug_wifi_ap_config_t ap_config = {
        .wifi_config = {
            .ap = {
                .ssid =             DEFAULT_AP_WIFI_SSID,
                .ssid_len =         strlen(DEFAULT_AP_WIFI_SSID),
    #if defined(CONFIG_AP_MANUAL_CHANNEL)
                .channel =          channel,
    #endif
                .password =         DEFAULT_AP_WIFI_PASSWORD,
                .max_connection =   DEFAULT_MAX_STA_CONN,
    #if defined(CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT)
                .authmode =         WIFI_AUTH_WPA3_PSK,
                .sae_pwe_h2e =      WPA3_SAE_PWE_BOTH,
    #else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
                .authmode =         WIFI_AUTH_WPA2_PSK,
    #endif
                .pmf_cfg = {
                        .required = true,
                },
            },
        },
        .timeout_idle =            DEFAULT_TIMEOUT_IDLE,
    };
    if (strlen(DEFAULT_AP_WIFI_PASSWORD) == 0)
        ap_config.wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    return ap_config;
}

#if defined(CONFIG_WIFI_INFO)
aug_wifi_sta_config_t aug_wifi_get_default_sta_config(void)
{
    aug_wifi_sta_config_t sta_config = {
        .wifi_config = {
            .sta = {
                .ssid =                 DEFAULT_STA_WIFI_SSID,
                .password =             DEFAULT_STA_WIFI_PASSWORD,
                /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
                * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
                * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
                * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
                */
                .threshold.authmode =   DEFAULT_WIFI_SCAN_AUTH_MODE_THRESHOLD,
                .sae_pwe_h2e =          DEFAULT_WIFI_SAE_MODE,
                .sae_h2e_identifier =   DEFAULT_H2E_IDENTIFIER,
            },
        },
        .max_retry = DEFAULT_MAXIMUM_RETRY,
    };

    return sta_config;
}
#endif

esp_err_t aug_wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing wifi needed resources");
    /* Initializes the TCP/IP stack
    Use esp_event_loop_delete_default() to free resources */
    AUG_RETURN_CHECK(esp_netif_init());
    /* Initializes the event loop required by sdk modules, including Wi-Fi ones
    Use esp_netif_deinit() to free resources */
    AUG_RETURN_CHECK(esp_event_loop_create_default());

    return ESP_OK;
}

esp_err_t aug_wifi_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing wifi needed resources");
    AUG_RETURN_CHECK(esp_event_loop_delete_default());
    //AUG_RETURN_CHECK(esp_netif_deinit());//     Note: Deinitialization is not supported yet

    return ESP_OK;
}