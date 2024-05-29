/**
 * @file aug_nvs.h
 * @brief Functions for handling non-volatile storage (NVS) operations 
 *        related to device configurations.
 */
#if !defined(AUG_NVS_H)
#define AUG_NVS_H

#include <esp_check.h>

/**
 * @brief Retrieves the Wi-Fi access point mode configuration stored in NVS memory
 *        and assigns it to the statically allocated configuration in the module.
 * @return 
 *      - ESP_OK: Success
 *      - Others: Refer to error codes in esp_err.h
 */
esp_err_t aug_nvs_get_ap_config(void);

/**
 * @brief Stores the Wi-Fi access point mode configuration from the statically allocated configuration
 *        in the module to the NVS memory.
 * @return esp_err_t 
 *      - ESP_OK: Success 
 *      - Others: Refer to error codes in esp_err.h
 */
esp_err_t aug_nvs_set_ap_config(void);

/**
 * @brief Retrieves the Wi-Fi station mode configuration stored in NVS memory
 *        and assigns it to the statically allocated configuration in the module.
 * @return 
 *      - ESP_OK: Success
 *      - Others: Refer to error codes in esp_err.h
 */
esp_err_t aug_nvs_get_sta_config(void);

/**
 * @brief Stores the Wi-Fi station mode configuration from the statically allocated configuration
 *        in the module to the NVS memory.
 * @return esp_err_t 
 *      - ESP_OK: Success 
 *      - Others: Refer to error codes in esp_err.h
 */
esp_err_t aug_nvs_set_sta_config(void);

/**
 * @brief Retrieves the MQTT client configuration stored in NVS memory
 *        and assigns it to the statically allocated configuration in the module.
 * @return esp_err_t 
 *      - ESP_OK: Success 
 *      - Others: Refer to error codes in esp_err.h 
 */
esp_err_t aug_nvs_get_mqtt_config(void);

/**
 * @brief Stores the MQTT client configuration from the statically allocated configuration
 *        in the module to the NVS memory.
 * @return esp_err_t 
 *      - ESP_OK: Success 
 *      - Others: Refer to error codes in esp_err.h
 */
esp_err_t aug_nvs_set_mqtt_config(void);

#endif
