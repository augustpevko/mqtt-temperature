/**
 * @file aug_wifi.h
 * @brief Retrieves default configurations and initializes common Wi-Fi resources.  
 * @warning Deallocation of resources is not working correctly.
 * @todo Make tests for multiple allocations-deallocations to check memleaks. 
 * @todo Add support for deallocation. 
 */

#if !defined(AUG_WIFI_H)
#define AUG_WIFI_H

#include <esp_wifi_types.h>

/**
 * @brief Keeps station config and max connection retries together.
 */
typedef struct {
    wifi_config_t wifi_config;
    int max_retry;
} aug_wifi_sta_config_t;

typedef struct {
    wifi_config_t wifi_config;
    int timeout_idle;
} aug_wifi_ap_config_t;

/**
 * @brief Retrieves the default Wi-Fi configuration for initializing an access point.
 * @return wifi_config_t A structure filled with default
 *         access point configuration values. The default values can be derived from
 *         project configuration settings, files, etc.
 */
aug_wifi_ap_config_t aug_wifi_get_default_ap_config(void);
/**
 * @brief Retrieves the default Wi-Fi configuration for initializing a station.
 * @return aug_wifi_sta_config_t A structure filled with default 
 *         station configuration values. The default values can be derived from
 *         project configuration settings, files, etc.
 */
aug_wifi_sta_config_t aug_wifi_get_default_sta_config(void);
/**
 * @brief Initializes common Wi-Fi resources required for both station and access point modes.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_wifi_init(void);
/**
 * @brief Deinitializes common Wi-Fi resources required for both station and access point modes.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 * @warning Deinitialization of common Wi-Fi resources is not supported by the SDK yet.
 */
esp_err_t aug_wifi_deinit(void);

#endif