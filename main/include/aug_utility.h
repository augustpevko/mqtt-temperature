/**
 * @file aug_utility.h
 * @brief Header file for utility functions.
 */
#if !defined(AUG_UTILITY_H)
#define AUG_UTILITY_H

#include <esp_check.h>
#include <esp_wifi_types.h>

#define MAX_OF_2(a, b) ((a) > (b) ? (a) : (b))
#define MAX_OF_4(a, b, c, d) \
    MAX_OF_2(MAX_OF_2(MAX_OF_2(a, b), c), d)
#define MAX_OF_8(a, b, c, d, e, f, g, h) \
    MAX_OF_2(MAX_OF_4(a, b, c, d), MAX_OF_4(e, f, g, h))

#define AUG_RETURN_CHECK(result) ({ \
        esp_err_t err = (result);   \
        if (err != ESP_OK)          \
            return err;             \
    })                              \

#define AUG_EXIT_CHECK(result) ({ \
        esp_err_t err = (result); \
        if (err != ESP_OK)        \
            ESP_ERROR_CHECK(err); \
    })                            \

#define AUG_EXIT_NULL_CHECK(result) if (result == NULL) ESP_ERROR_CHECK(ESP_FAIL)

/**
 * @brief Converts a string representation of Wi-Fi authentication mode to its corresponding enum.
 * @param buffer the buffer containing the string to be converted.
 * @param buffer_len length of the buffer.
 * @param auth_mode pointer to store the resulting Wi-Fi authentication mode enum.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_str_to_auth_mode(const char* buffer, size_t buffer_len, wifi_auth_mode_t* auth_mode);

/**
 * @brief Converts Wi-Fi authentication mode enum to its corresponding string representation.
 * @param auth_mode the Wi-Fi authentication mode enum.
 * @param buffer buffer to store the resulting string.
 * @param buffer_size size of the buffer.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_auth_mode_to_str(wifi_auth_mode_t auth_mode, char* buffer, size_t buffer_size);

/**
 * @brief Converts a string representation of SAE mode to its corresponding enum.
 * @param buffer the buffer containing the string to be converted.
 * @param buffer_len length of the buffer.
 * @param sae_mode pointer to store the resulting SAE mode enum.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_str_to_sae_mode(const char* buffer, size_t buffer_len, wifi_sae_pwe_method_t* sae_mode);

/**
 * @brief Converts SAE mode enum to its corresponding string representation.
 * @param sae_mode the SAE mode enum.
 * @param buffer buffer to store the resulting string.
 * @param buffer_size size of the buffer.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_sae_mode_to_str(wifi_sae_pwe_method_t sae_mode, char* buffer, size_t buffer_size);

/**
 * @brief Gets the maximum size required to store a Wi-Fi authentication mode string.
 * @return Size required to store a Wi-Fi authentication mode string.
 */
size_t aug_get_auth_mode_size();

/**
 * @brief Gets the maximum size required to store an SAE mode string.
 * @return Size required to store an SAE mode string.
 */
size_t aug_get_sae_mode_size();

#endif