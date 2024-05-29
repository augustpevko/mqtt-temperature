#include "aug_utility.h"

#include <string.h>

static const char                  AUG_AUTH_OPEN[] = "open";
static const char                  AUG_AUTH_WEP[] = "wep";
static const char                  AUG_AUTH_WPA_PSK[] = "wpa_psk";
static const char                  AUG_AUTH_WPA2_PSK[] = "wpa2_psk";
static const char                  AUG_AUTH_WPA_WPA2_PSK[] = "wpa_wpa2_psk";
static const char                  AUG_AUTH_WPA3_PSK[] = "wpa3_psk";
static const char                  AUG_AUTH_WPA2_WPA3_PSK[] = "wpa2_wpa3_psk";
static const char                  AUG_AUTH_WAPI_PSK[] = "wapi_psk";
static const size_t                AUG_AUTH_MODE_MAX_SIZE = MAX_OF_8(
    sizeof(AUG_AUTH_OPEN), sizeof(AUG_AUTH_WEP), 
    sizeof(AUG_AUTH_WPA_PSK), sizeof(AUG_AUTH_WPA2_PSK), 
    sizeof(AUG_AUTH_WPA_WPA2_PSK), sizeof(AUG_AUTH_WPA3_PSK),
    sizeof(AUG_AUTH_WPA2_WPA3_PSK), sizeof(AUG_AUTH_WAPI_PSK));
static const wifi_auth_mode_t      AUG_AUTH_MODES_ENUM[] = { 
    WIFI_AUTH_OPEN, WIFI_AUTH_WEP, 
    WIFI_AUTH_WPA_PSK, WIFI_AUTH_WPA2_PSK, 
    WIFI_AUTH_WPA_WPA2_PSK, WIFI_AUTH_WPA3_PSK, 
    WIFI_AUTH_WPA2_WPA3_PSK, WIFI_AUTH_WAPI_PSK };
static const char*                 AUG_AUTH_MODES_STR[] = { 
    AUG_AUTH_OPEN, AUG_AUTH_WEP, 
    AUG_AUTH_WPA_PSK, AUG_AUTH_WPA2_PSK, 
    AUG_AUTH_WPA_WPA2_PSK, AUG_AUTH_WPA3_PSK, 
    AUG_AUTH_WPA2_WPA3_PSK, AUG_AUTH_WAPI_PSK };

static const char                  AUG_SAE_MODE_UNSPECIFIED[] = "unspecified";
static const char                  AUG_SAE_MODE_HUNT_AND_PECK[] = "hunt_and_peck";
static const char                  AUG_SAE_MODE_H2E[] = "h2e";
static const char                  AUG_SAE_MODE_BOTH[] = "both";
static const size_t                AUG_SAE_MODE_MAX_SIZE = MAX_OF_4(
    sizeof(AUG_SAE_MODE_UNSPECIFIED), sizeof(AUG_SAE_MODE_HUNT_AND_PECK), 
    sizeof(AUG_SAE_MODE_H2E), sizeof(AUG_SAE_MODE_BOTH));
static const wifi_sae_pwe_method_t AUG_SAE_MODES_ENUM[] = { 
    WPA3_SAE_PWE_UNSPECIFIED, WPA3_SAE_PWE_HUNT_AND_PECK, 
    WPA3_SAE_PWE_HASH_TO_ELEMENT, WPA3_SAE_PWE_BOTH };
static const char*                 AUG_SAE_MODES_STR[] = { 
    AUG_SAE_MODE_UNSPECIFIED, AUG_SAE_MODE_HUNT_AND_PECK, 
    AUG_SAE_MODE_H2E, AUG_SAE_MODE_BOTH };

static const char *TAG = "utility";

static int string_cmp(const char *str1, size_t len1, const char *str2, size_t len2) {
    if (len1 != len2)
        return 0;
    return strncmp(str1, str2, len2) == 0;
}

esp_err_t aug_str_to_auth_mode(const char* buffer, size_t buffer_len, wifi_auth_mode_t* auth_mode) {
    const size_t size_strs = sizeof(AUG_AUTH_MODES_STR) / sizeof(*AUG_AUTH_MODES_STR);
    esp_err_t found = ESP_FAIL;
    for (size_t i = 0; i < size_strs; ++i) {
        if (string_cmp(buffer, buffer_len, AUG_AUTH_MODES_STR[i], strlen(AUG_AUTH_MODES_STR[i]))) {
            *auth_mode = AUG_AUTH_MODES_ENUM[i];
            found = ESP_OK;
            break;
        }
    }
    return found;
}

esp_err_t aug_auth_mode_to_str(wifi_auth_mode_t auth_mode, char* buffer, size_t buffer_size)
{
    if (buffer_size < AUG_AUTH_MODE_MAX_SIZE) {
        ESP_LOGI(TAG, "the buffer size is insufficient");
        return ESP_FAIL;
    }
    const size_t size_enum = sizeof(AUG_AUTH_MODES_ENUM) / sizeof(*AUG_AUTH_MODES_ENUM);
    for (size_t i = 0; i < size_enum; ++i) {
        if (AUG_AUTH_MODES_ENUM[i] == auth_mode) {
            ESP_LOGI(TAG, "the %s was found, converting to str", AUG_AUTH_MODES_STR[i]);
            size_t mode_str_size = strlen(AUG_AUTH_MODES_STR[i]) + 1;//+1 for \0
            memcpy(buffer, AUG_AUTH_MODES_STR[i], mode_str_size);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t aug_str_to_sae_mode(const char* buffer, size_t buffer_len, wifi_sae_pwe_method_t* sae_mode) {
    const size_t size_strs = sizeof(AUG_SAE_MODES_STR) / sizeof(*AUG_SAE_MODES_STR);
    esp_err_t found = ESP_FAIL;
    for (size_t i = 0; i < size_strs; ++i) {
        if (string_cmp(buffer, buffer_len, AUG_SAE_MODES_STR[i], strlen(AUG_SAE_MODES_STR[i]))) {
            *sae_mode = AUG_SAE_MODES_ENUM[i];
            found = ESP_OK;
            break;
        }
    }
    return found;
}

esp_err_t aug_sae_mode_to_str(wifi_sae_pwe_method_t sae_mode, char* buffer, size_t buffer_size)
{
    if (buffer_size < AUG_SAE_MODE_MAX_SIZE) {
        ESP_LOGI(TAG, "the buffer size is insufficient");
        return ESP_FAIL;
    }
    const size_t size_enum = sizeof(AUG_SAE_MODES_ENUM) / sizeof(*AUG_SAE_MODES_ENUM);
    for (size_t i = 0; i < size_enum; ++i) {
        if (AUG_SAE_MODES_ENUM[i] == sae_mode) {
            ESP_LOGI(TAG, "the %s was found, converting to str", AUG_SAE_MODES_STR[i]);
            size_t mode_str_size = strlen(AUG_SAE_MODES_STR[i]) + 1;//+1 for \0
            memcpy(buffer, AUG_SAE_MODES_STR[i], mode_str_size);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

size_t aug_get_auth_mode_size()
{
    return AUG_AUTH_MODE_MAX_SIZE;
}

size_t aug_get_sae_mode_size()
{
    return AUG_SAE_MODE_MAX_SIZE;
}