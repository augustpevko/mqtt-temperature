/**
 * @file aug_mqtt_client.h
 * @brief Initializes mqtt client, connects to broker,
 *        publishes data with predefined quality of service.
 * @todo Make tests for multiple allocations-deallocations to check memleaks. 
 */

#if !defined(AUG_MQTT_CLIENT_H)
#define AUG_MQTT_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

#include <esp_check.h>
#include <mqtt_client.h>

#define MQTT_MAX_URI_LEN CONFIG_HTTPD_MAX_URI_LEN
#define DEFAULT_MQTT_BROKER_URI CONFIG_BROKER_URI
#define DEFAULT_PUBLISH_RATE CONFIG_PUBLISH_RATE

/**
 * @brief Structure needed because the MQTT configuration structure
 *        doesn't allocate memory for the broker URI.  
 */
typedef struct {
    char* uri_str;
    size_t uri_len;
} aug_mqtt_uri_t;

/**
 * @brief Initializes a MQTT client.
 * Allocates resources that should be freed with aug_mqtt_deinit.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_mqtt_init(void);
/**
 * @brief Deinitializes a MQTT client.
 * Deallocates resources that was allocated with aug_mqtt_init.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h 
 */
esp_err_t aug_mqtt_deinit(void);
/**
 * @brief Starts already initialized MQTT client.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_mqtt_start(void);
/**
 * @brief Stops already initialized MQTT client.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h 
 */
esp_err_t aug_mqtt_stop(void);
/**
 * @brief Stops MQTT client, sets the MQTT broker URI and starts MQTT client. 
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h 
 */
esp_err_t aug_mqtt_set_uri(void);
/**
 * @brief Returns the MQTT broker uri from statically allocated configuration in module.
 * @return aug_mqtt_uri_t Structure keeps pointer to uri buffer with length of this buffer.
 */
aug_mqtt_uri_t aug_mqtt_get_uri(void);
/**
 * @brief Sets the default MQTT broker URI.
 */
void aug_mqtt_set_default_uri(void);
/**
 * @brief Publishes data to the topic with predefined qos in module.
 * @param topic Topic string that should be null-terminated.
 * @param data Data string that should be null-terminated.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 */
esp_err_t aug_mqtt_publish_str(const char* topic, const char* data);
/**
 * @brief Returns the current state of this module.
 * @return true If the module is initialized.
 * @return false If the module is not initialized.
 */
bool aug_mqtt_is_init(void);
/**
 * @brief Returns the current state of the MQTT client connection.
 * @return true If the MQTT client is connected.
 * @return false If the MQTT client is not connected.
 */
bool aug_mqtt_is_connected(void);
/**
 * @brief Returns a pointer to the statically allocated the MQTT client configuration.
 * @return esp_mqtt_client_config_t* Pointer to statically allocated structure. 
 */
esp_mqtt_client_config_t* aug_mqtt_get_config(void);

#endif