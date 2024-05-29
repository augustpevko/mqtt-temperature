/**
 * @file aug_wifi_ap.h
 * @brief Initializes Wi-Fi access point.  
 * @todo Make tests for multiple allocations-deallocations to check memleaks. 
 */

#if !defined(AUG_WIFI_AP_H)
#define AUG_WIFI_AP_H

#include <stdbool.h>

#include <esp_wifi_types.h>

#include "aug_wifi.h"

#define DEFAULT_TIMEOUT_IDLE CONFIG_TIMEOUT_IDLE

/**
 * @brief Constructs a new esp event declare base object
 *        for publishing access point mode events.
 */
ESP_EVENT_DECLARE_BASE(AUG_WIFI_AP_EVENTS);
enum {
    /**
     * @brief Event published when the timeout has passed and nobody is connected to the access point.
     */
    AUG_WIFI_AP_EVENT_IDLE,
};

/**
 * @brief Starts Wi-Fi access point.
 * Allocates resources that should be freed with aug_wifi_ap_stop.
 * @param _event_loop_handle Pointer to the event loop to publish events to.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_wifi_ap_start(esp_event_loop_handle_t* _event_loop_handle);
/**
 * @brief Stops Wi-Fi access point.
 * Deallocates resources that was allocated with aug_wifi_ap_start.
 * @return esp_err_t
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_wifi_ap_stop(void);
/**
 * @brief Returns the current state of this module.
 * @return true If the module is initialized.
 * @return false If the module is not initialized.
 */
bool aug_wifi_ap_is_init(void);
/**
 * @brief Returns a pointer to the statically allocated access point configuration.
 * @return aug_wifi_ap_config_t* Pointer to statically allocated structure. 
 */
aug_wifi_ap_config_t* aug_wifi_ap_get_config(void);

#endif