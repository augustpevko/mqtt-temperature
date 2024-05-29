#include "aug_mqtt_client.h"

#include <mqtt_client.h>
#include <esp_event.h>
#include <esp_check.h>

#include "aug_utility.h"

static const char *TAG = "mqtt client";

static char uri_str[MQTT_MAX_URI_LEN + 1] = {};
static esp_mqtt_client_config_t mqtt_config = {};
static esp_mqtt_client_handle_t mqtt_client_handle = NULL;
static bool is_connected = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        is_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        is_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t aug_mqtt_init(void)
{
    ESP_LOGI(TAG, "Initializing mqtt client");
    mqtt_client_handle = esp_mqtt_client_init(&mqtt_config);
    if (mqtt_client_handle == NULL) {
        ESP_LOGI(TAG, "error initializing mqtt client");
        return ESP_FAIL;
    }
    AUG_RETURN_CHECK(esp_mqtt_client_register_event(mqtt_client_handle, 
        ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));

    return ESP_OK;
}

esp_err_t aug_mqtt_deinit(void)
{
    AUG_RETURN_CHECK(esp_mqtt_client_destroy(mqtt_client_handle));
    mqtt_client_handle = NULL;
    is_connected = false;

    return ESP_OK;
}

esp_err_t aug_mqtt_start(void)
{
    return esp_mqtt_client_start(mqtt_client_handle);
}

esp_err_t aug_mqtt_stop(void)
{
    return esp_mqtt_client_stop(mqtt_client_handle);
}

esp_err_t aug_mqtt_set_uri(void)
{
    AUG_RETURN_CHECK(esp_mqtt_client_stop(mqtt_client_handle));
    AUG_RETURN_CHECK(esp_mqtt_client_set_uri(mqtt_client_handle, uri_str));
    AUG_RETURN_CHECK(esp_mqtt_client_start(mqtt_client_handle));

    return ESP_OK;
}

aug_mqtt_uri_t aug_mqtt_get_uri(void)
{
    return (aug_mqtt_uri_t){ .uri_str = uri_str, 
        .uri_len = MQTT_MAX_URI_LEN };
}

void aug_mqtt_set_default_uri(void)
{
    static_assert(sizeof(DEFAULT_MQTT_BROKER_URI) <= sizeof(uri_str),
        "the broker uri exceeds the maximum buffer size");
    memset(uri_str, 0, sizeof(uri_str));
    memcpy(uri_str, DEFAULT_MQTT_BROKER_URI, sizeof(DEFAULT_MQTT_BROKER_URI));
}

esp_err_t aug_mqtt_publish_str(const char* topic, const char* data)
{
    int msg_id = esp_mqtt_client_publish(mqtt_client_handle, topic, data, 0, 0, 0);
    ESP_LOGI(TAG, "Sent publish successful, msg_id=%d, topic=%s, data=%s", msg_id, topic, data);
    return ESP_OK;
}

bool aug_mqtt_is_init(void)
{
    if (mqtt_client_handle)
        return true;
    return false;
}

bool aug_mqtt_is_connected(void)
{
    return is_connected;
}

esp_mqtt_client_config_t* aug_mqtt_get_config(void)
{
    return &mqtt_config;
}