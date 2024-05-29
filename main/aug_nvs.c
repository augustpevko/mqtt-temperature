#include "aug_nvs.h"

#include <string.h>
#include <stdint.h>

#include <nvs_flash.h>

#include "aug_wifi_ap.h"
#include "aug_wifi_sta.h"
#include "aug_mqtt_client.h"

static const char wifi_ap_config_namespace_str[] = "wifi_ap_config";
static const char wifi_ap_config_config_str[] = "config";

static const char wifi_sta_config_namespace_str[] = "wifi_sta_config";
static const char wifi_sta_config_config_str[] = "config";

static const char mqtt_config_namespace_str[] = "mqtt_config";
static const char mqtt_config_config_str[] = "uri";

static esp_err_t get_blob(const char* namespace, const char* str, 
    uint8_t* buffer, const size_t buffer_size)
{
    nvs_handle_t nvs_handle = 0;
    esp_err_t err = ESP_FAIL;
    uint8_t* data = NULL;

    err = nvs_open(namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
        goto out;

    size_t size_in_nvs = 0;
    err = nvs_get_blob(nvs_handle, str, 
        NULL, &size_in_nvs);
    if (err != ESP_OK)
        goto out;

    if (size_in_nvs != buffer_size) {
        err = ESP_FAIL;
        goto out;
    }
    data = (uint8_t*)malloc(size_in_nvs);
    if (!data) {
        err = ESP_ERR_NO_MEM;
        goto out;
    }
    err = nvs_get_blob(nvs_handle, str, 
        data, &size_in_nvs);
    if (err != ESP_OK)
        goto out;
    memcpy(buffer, data, buffer_size);
    err = ESP_OK;

out:
    if (data != NULL)
        free(data);
    nvs_close(nvs_handle);
    return err;          
}

static esp_err_t set_blob(const char* namespace, const char* str, 
    const uint8_t* buffer, const size_t buffer_size)
{
    nvs_handle_t nvs_handle = 0;
    esp_err_t err = ESP_FAIL;
    
    err = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        goto out;
    err = nvs_set_blob(nvs_handle, str,
        buffer, buffer_size);
    if (err != ESP_OK)
        goto out;
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
        goto out;
    err = ESP_OK;    

out:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t aug_nvs_get_ap_config(void)
{
    aug_wifi_ap_config_t* ap_config = aug_wifi_ap_get_config();
    return get_blob(wifi_ap_config_namespace_str, wifi_ap_config_config_str,
        (uint8_t*)ap_config, sizeof(*ap_config));
}

esp_err_t aug_nvs_set_ap_config(void)
{
    aug_wifi_ap_config_t* ap_config = aug_wifi_ap_get_config();
    return set_blob(wifi_ap_config_namespace_str, wifi_ap_config_config_str,
        (const uint8_t*)ap_config, sizeof(*ap_config));
}

esp_err_t aug_nvs_get_sta_config(void)
{
    aug_wifi_sta_config_t* sta_config = aug_wifi_sta_get_config();
    return get_blob(wifi_sta_config_namespace_str, wifi_sta_config_config_str,
        (uint8_t*)sta_config, sizeof(*sta_config));
}

esp_err_t aug_nvs_set_sta_config(void)
{
    aug_wifi_sta_config_t* sta_config = aug_wifi_sta_get_config();
    return set_blob(wifi_sta_config_namespace_str, wifi_sta_config_config_str,
        (const uint8_t*)sta_config, sizeof(*sta_config));
}

esp_err_t aug_nvs_get_mqtt_config(void)
{
    aug_mqtt_uri_t mqtt_uri = aug_mqtt_get_uri();
    return get_blob(mqtt_config_namespace_str, mqtt_config_config_str,
        (uint8_t*)mqtt_uri.uri_str, mqtt_uri.uri_len);
}

esp_err_t aug_nvs_set_mqtt_config(void)
{
    aug_mqtt_uri_t mqtt_uri = aug_mqtt_get_uri();
    return set_blob(mqtt_config_namespace_str, mqtt_config_config_str,
        (const uint8_t*)mqtt_uri.uri_str, mqtt_uri.uri_len);
}