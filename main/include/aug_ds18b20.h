/**
 * @file aug_ds18b20.h
 * @brief Finds connected sensors in the hardware setup 
 *        and retrieves the current temperature.
 */

#if !defined(AUG_DS18B20_H)
#define AUG_DS18B20_H

#include <stddef.h>
#include <esp_check.h>

/**
 * @brief Initializes the DS18B20 sensor driver, searches for sensors, and sets resolution.
 *        Initializes resources that should be cleaned up with aug_ds18b20_deinit.
 * @return esp_err_t
 *      - ESP_OK: Succeeds 
 *      - others: Refer to error codes in esp_err.h
 */
esp_err_t aug_ds18b20_init();
/**
 * @brief Deinitializes resources related to the DS18B20 sensor driver 
 *        that were initialized in aug_ds18b20_init.
 * @return esp_err_t
 *      - ESP_OK: Succeeds 
 */
esp_err_t aug_ds18b20_deinit();
/**
 * @brief Returns the number of found sensors.
 * @return size_t Number of found sensors.
 */
size_t aug_get_sensors_number();
/**
 * @brief Returns the current temperature by the sensor index.
 * @param index Sensor index.
 * @return float Current temperature in Celsius. 
 */
float aug_get_temperature(size_t index);

#endif
