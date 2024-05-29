#include "aug_http_server.h"

#include <esp_http_server.h>
#include <esp_wifi_types.h>
#include <esp_log.h>
#include <esp_ota_ops.h>

#include "aug_utility.h"
#include "aug_wifi_sta.h"
#include "aug_mqtt_client.h"

static const char *TAG = "http server";

extern const char index_start[] asm("_binary_index_html_start");
extern const char index_end[] asm("_binary_index_html_end");

static httpd_handle_t server = NULL;

ESP_EVENT_DEFINE_BASE(AUG_HTTP_SERVER_EVENTS);

#define OTA_CHUNK_SIZE 1024

static void send_bad_request_msg(const char* msg, size_t msg_len, 
    const char* option_str, size_t option_len, httpd_req_t *req)
{
    size_t total_size = msg_len + option_len + 1;//+1 for \0
    char result_message[total_size] = {};
    snprintf(result_message, total_size, msg, option_str);
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_sendstr(req, result_message);
}

static void send_unexpected_error(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Unexpected error");
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_sendstr(req, "<div>Unexpected error</div>\r\n");
} 

static esp_err_t set_str_value(httpd_req_t *req, char* query_str, 
    const char* option_str, void* option_buffer, size_t buffer_size)
{
    char buffer[buffer_size] = {};
    esp_err_t result = httpd_query_key_value(query_str, option_str, buffer, buffer_size);
    switch (result) {
        case ESP_ERR_HTTPD_RESULT_TRUNC: {
            ESP_LOGI(TAG, "The %s length exceeds the limit", option_str);
            const char msg[] = "<div>The %s length exceeds the limit</div>\r\n";
            send_bad_request_msg(msg, sizeof(msg) - 2, option_str, strlen(option_str), req);//-2 for %s
            return ESP_ERR_HTTPD_RESULT_TRUNC;
        }
        case ESP_ERR_NOT_FOUND:
            ESP_LOGI(TAG, "The %s wasn't found", option_str);
            break;
        case ESP_OK: {
            ESP_LOGI(TAG, "The %s was found, memcpy in struct", option_str);
            memcpy(option_buffer, buffer, buffer_size);
            break;
        }
        default:
            send_unexpected_error(req);
            return result;
    }
    return ESP_OK;
}

static esp_err_t set_int_value(httpd_req_t *req, char* query_str, 
    const char* option_str, int* option_number)
{
    const size_t max_int_chars = 24;
    char buffer[max_int_chars] = {};
    esp_err_t result = httpd_query_key_value(query_str, option_str, buffer, sizeof(buffer));
    
    switch (result) {
        case ESP_ERR_HTTPD_RESULT_TRUNC: {
            ESP_LOGI(TAG, "The %s length exceeds the limit", option_str);
            const char msg[] = "<div>The %s length exceeds the limit</div>\r\n";
            send_bad_request_msg(msg, sizeof(msg) - 2, option_str, strlen(option_str), req);//-2 for %s
            return ESP_ERR_HTTPD_RESULT_TRUNC;
        }
        case ESP_ERR_NOT_FOUND:
            ESP_LOGI(TAG, "The %s wasn't found", option_str);
            break;
        case ESP_OK: {
            ESP_LOGI(TAG, "The %s was found, converting to integer", option_str);
            *option_number = atoi(buffer);
            break;
        }
        default:
            send_unexpected_error(req);
            return result;
    }
    return ESP_OK;
}

static esp_err_t set_auth_enum_value(httpd_req_t *req, char* query_str, 
    const char* option_str, wifi_auth_mode_t* option_enum)
{    
    const size_t auth_mode_size = aug_get_auth_mode_size();
    char buffer[auth_mode_size] = {};
    esp_err_t result = httpd_query_key_value(query_str, option_str, buffer, sizeof(buffer));

    switch (result) {
        case ESP_ERR_HTTPD_RESULT_TRUNC: {
            ESP_LOGI(TAG, "The %s length exceeds the limit", option_str);
            const char msg[] = "<div>The %s length exceeds the limit</div>\r\n";
            send_bad_request_msg(msg, sizeof(msg) - 2, option_str, strlen(option_str), req);//-2 for %s
            return ESP_ERR_HTTPD_RESULT_TRUNC;
        }
        case ESP_ERR_NOT_FOUND:
            ESP_LOGI(TAG, "The %s wasn't found", option_str);
            break;
        case ESP_OK: {
            esp_err_t found = aug_str_to_auth_mode(buffer, strlen(buffer), option_enum);
            if (found != ESP_OK) {
                ESP_LOGI(TAG, "The %s has invalid value", option_str);
                const char msg[] = "<div>The %s has invalid value</div>\r\n";
                send_bad_request_msg(msg, sizeof(msg) - 2, option_str, strlen(option_str), req);//-2 for %s
                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "The %s was found, converting to enum", option_str);
            break;
        }
        default:
            send_unexpected_error(req);
            return result;
    }
    return ESP_OK;
}

static esp_err_t set_sae_mode_enum_value(httpd_req_t *req, char* query_str, 
    const char* option_str, wifi_sae_pwe_method_t* option_enum)
{
    const size_t sae_mode_size = aug_get_sae_mode_size();
    char buffer[sae_mode_size] = {};
    esp_err_t result = httpd_query_key_value(query_str, option_str, buffer, sizeof(buffer));

    switch (result) {
        case ESP_ERR_HTTPD_RESULT_TRUNC: {
            ESP_LOGI(TAG, "The %s length exceeds the limit", option_str);
            const char msg[] = "<div>The %s length exceeds the limit</div>\r\n";
            send_bad_request_msg(msg, sizeof(msg) - 2, option_str, strlen(option_str), req);//-2 for %s
            return ESP_ERR_HTTPD_RESULT_TRUNC;
        }
        case ESP_ERR_NOT_FOUND:
            ESP_LOGI(TAG, "The %s wasn't found", option_str);
            break;
        case ESP_OK: {
            esp_err_t found = aug_str_to_sae_mode(buffer, strlen(buffer), option_enum);
            if (found != ESP_OK) {
                ESP_LOGI(TAG, "The %s has invalid value", option_str);
                const char msg[] = "<div>The %s has invalid value</div>\r\n";
                send_bad_request_msg(msg, sizeof(msg) - 2, option_str, strlen(option_str), req);//-2 for %s
                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "The %s was found, converting to enum", option_str);
            break;
        }
        default:
            send_unexpected_error(req);
            return result;
    }
    return ESP_OK;
}

static esp_err_t set_sta_options(httpd_req_t *req, char* query_str, aug_wifi_sta_config_t* sta_config)
{
    const char ssid_option[] =        "ssid";
    const char password_option[] =    "password";
    const char max_retry_option[] =   "max_retry";
    const char auth_mode_option[] =   "auth_mode";
    const char sae_mode_option[] =    "sae_mode";
    const char password_id_option[] = "password_id";

    size_t buffer_size;
    buffer_size = sizeof(sta_config->wifi_config.sta.ssid);
    AUG_RETURN_CHECK(set_str_value(req, query_str, ssid_option, 
        sta_config->wifi_config.sta.ssid, buffer_size));
    buffer_size = sizeof(sta_config->wifi_config.sta.password);
    AUG_RETURN_CHECK(set_str_value(req, query_str, password_option,
        sta_config->wifi_config.sta.password, buffer_size));
    AUG_RETURN_CHECK(set_int_value(req, query_str, 
        max_retry_option, &sta_config->max_retry));
    AUG_RETURN_CHECK(set_auth_enum_value(req, query_str, 
        auth_mode_option, &sta_config->wifi_config.sta.threshold.authmode));
    wifi_sae_pwe_method_t* sae_mode_ptr = &sta_config->wifi_config.sta.sae_pwe_h2e;
    AUG_RETURN_CHECK(set_sae_mode_enum_value(req, query_str, 
        sae_mode_option, sae_mode_ptr));
    if (*sae_mode_ptr == WPA3_SAE_PWE_HASH_TO_ELEMENT || *sae_mode_ptr == WPA3_SAE_PWE_BOTH) {
        buffer_size = sizeof(sta_config->wifi_config.sta.sae_h2e_identifier);
        AUG_RETURN_CHECK(set_str_value(req, query_str,
            password_id_option, sta_config->wifi_config.sta.sae_h2e_identifier, buffer_size));
    }
    else
        ESP_LOGI(TAG, "Skipping h2e identifier because sae mode is not set so");
    
    return ESP_OK;
}

static esp_err_t set_options_sta_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /set_options/sta");
    size_t query_size = httpd_req_get_url_query_len(req);
    char query_str[query_size + 1] = {};
    if (httpd_req_get_url_query_str(req, query_str, query_size + 1) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Query: %s", query_str);
    
    aug_wifi_sta_config_t* sta_config = aug_wifi_sta_get_config();
    if (set_sta_options(req, query_str, sta_config) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Options are set");
    
    httpd_resp_set_status(req, "200 Success");
    return httpd_resp_sendstr(req, "<div>Options are set</div>\r\n");
}

static esp_err_t send_sta_info(httpd_req_t *req, aug_wifi_sta_config_t* sta_config) {
    const size_t auth_mode_size = aug_get_auth_mode_size();
    const size_t sae_mode_size = aug_get_sae_mode_size();
    char auth_mode_str[auth_mode_size] = {};
    char sae_mode_str[sae_mode_size] = {};
    if (aug_auth_mode_to_str(sta_config->wifi_config.sta.threshold.authmode, 
            auth_mode_str, sizeof(auth_mode_str)) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    if (aug_sae_mode_to_str(sta_config->wifi_config.sta.sae_pwe_h2e, 
            sae_mode_str, sizeof(sae_mode_str)) != ESP_OK)  {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Trying to connect to sta with options:\n"
        "ssid: %s\n"
        "password: %s\n"
        "max retries: %d\n"
        "auth mode: %s\n"
        "sae mode: %s\n"
        "password id: %s",
        sta_config->wifi_config.sta.ssid, sta_config->wifi_config.sta.password, sta_config->max_retry,
        auth_mode_str, sae_mode_str, sta_config->wifi_config.sta.sae_h2e_identifier
    );
    const size_t max_int_chars = 24;
    const char info_str[] = "<div>Trying to connect to sta with options:<br>\r\n"
        "ssid: <br>\r\n"
        "password: <br>\r\n"
        "max retries: <br>\r\n"
        "auth mode: <br>\r\n"
        "sae mode: <br>\r\n"
        "password id: </div><br>\r\n";
    const size_t formatted_string_max_size = sizeof(sta_config->wifi_config.sta.ssid) 
        + sizeof(sta_config->wifi_config.sta.password)
        + max_int_chars
        + sizeof(auth_mode_str)
        + sizeof(sae_mode_str)
        + sizeof(sta_config->wifi_config.sta.sae_h2e_identifier)
        + sizeof(info_str);
    char buffer[formatted_string_max_size] = {};
    snprintf(buffer, formatted_string_max_size,
        "<div>Trying to connect to sta with options:<br>\r\n"
        "ssid: %s<br>\r\n"
        "password: %s<br>\r\n"
        "max retries: %d<br>\r\n"
        "auth mode: %s<br>\r\n"
        "sae mode: %s<br>\r\n"
        "password id: %s</div><br>\r\n",
        sta_config->wifi_config.sta.ssid, sta_config->wifi_config.sta.password, sta_config->max_retry,
        auth_mode_str, sae_mode_str, sta_config->wifi_config.sta.sae_h2e_identifier
    );
    
    httpd_resp_set_status(req, "200 Success");
    return httpd_resp_sendstr(req, buffer);
}

static esp_err_t init_sta_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /init/sta");
    esp_event_loop_handle_t* event_loop_handle = (esp_event_loop_handle_t*)req->user_ctx;
    aug_wifi_sta_config_t* sta_config = aug_wifi_sta_get_config();
    if (send_sta_info(req, sta_config) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    if (esp_event_post_to(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
            AUG_HTTP_SERVER_EVENT_INIT_STA, NULL, 0, portMAX_DELAY) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t set_options_mqtt(httpd_req_t *req, char* query_str, aug_mqtt_uri_t* mqtt_uri)
{
    const char uri_option[] = "uri";

    AUG_RETURN_CHECK(set_str_value(req, query_str, uri_option,
        mqtt_uri->uri_str, mqtt_uri->uri_len));

    return ESP_OK;
}

static esp_err_t set_options_mqtt_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /set_options/mqtt");
    size_t query_size = httpd_req_get_url_query_len(req);
    char query_str[query_size + 1] = {};
    if (httpd_req_get_url_query_str(req, query_str, query_size + 1) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Query: %s", query_str);

    aug_mqtt_uri_t mqtt_uri = aug_mqtt_get_uri();
    if (set_options_mqtt(req, query_str, &mqtt_uri) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Options are set");
    
    httpd_resp_set_status(req, "200 Success");
    return httpd_resp_sendstr(req, "<div>Options are set</div>\r\n");
}

static esp_err_t send_mqtt_info(httpd_req_t *req, aug_mqtt_uri_t* mqtt_uri)
{
    const char info_str[] = "<div>Trying to connect to mqtt broker with options:<br>\r\n"
        "URI: </div><br>\r\n";
    const size_t formatted_string_max_size = mqtt_uri->uri_len
        + sizeof(info_str)
        + 1;
    char buffer[formatted_string_max_size] = {};
    snprintf(buffer, formatted_string_max_size,
        "<div>Trying to connect to mqtt broker with options:<br>\r\n"
        "URI: %s</div><br>\r\n",
        mqtt_uri->uri_str);
    ESP_LOGI(TAG, "Buffer in hex:");

    httpd_resp_set_status(req, "200 Success");
    return httpd_resp_sendstr(req, buffer);
}

static esp_err_t init_mqtt_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /init/mqtt");
    esp_event_loop_handle_t* event_loop_handle = (esp_event_loop_handle_t*)req->user_ctx;
    aug_mqtt_uri_t mqtt_uri = aug_mqtt_get_uri();
    if (send_mqtt_info(req, &mqtt_uri) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    } 
    if (esp_event_post_to(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
            AUG_HTTP_SERVER_EVENT_INIT_MQTT, NULL, 0, portMAX_DELAY) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t ota_update_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /ota_update");
    esp_event_loop_handle_t* event_loop_handle = (esp_event_loop_handle_t*)req->user_ctx;
    
	esp_ota_handle_t ota_handle;
	const esp_partition_t* ota_partition = esp_ota_get_next_update_partition(NULL);
	ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));

    char buffer[OTA_CHUNK_SIZE];
	size_t content_len = req->content_len;
	while (content_len > 0) {
		int sent = httpd_req_recv(req, buffer,
            content_len < OTA_CHUNK_SIZE ? content_len : OTA_CHUNK_SIZE);
		if (sent == HTTPD_SOCK_ERR_TIMEOUT) {
			continue;
		}
        else if (sent <= 0) {
			send_unexpected_error(req);
			return ESP_FAIL;
		}
		
        if (esp_ota_write(ota_handle, buffer, sent) != ESP_OK) {
			send_unexpected_error(req);
			return ESP_FAIL;
		}
		content_len -= sent;
	}
	
    if (esp_ota_end(ota_handle) != ESP_OK 
            || esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
		send_unexpected_error(req);
        return ESP_FAIL;
	}
    if (esp_event_post_to(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
            AUG_HTTP_SERVER_EVENT_OTA_UPDATE, NULL, 0, portMAX_DELAY) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }

    httpd_resp_set_status(req, "200 Success");
    return httpd_resp_sendstr(req, "<div>Loading firmware complete</div>\r\n");
}

static esp_err_t restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /restart");
    esp_event_loop_handle_t* event_loop_handle = (esp_event_loop_handle_t*)req->user_ctx;

    if (esp_event_post_to(*event_loop_handle, AUG_HTTP_SERVER_EVENTS, 
            AUG_HTTP_SERVER_EVENT_RESTART, NULL, 0, portMAX_DELAY) != ESP_OK) {
        send_unexpected_error(req);
        return ESP_FAIL;
    }

    httpd_resp_set_status(req, "200 Success");
    return httpd_resp_sendstr(req, "<div>Rebooting...</div>\r\n");
}

static esp_err_t index_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URI: /");
    const size_t index_len = index_end - index_start;

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_start, index_len);
}

/**
 * @brief Registers a handler to set options for the station mode module.
 * @return esp_err_t
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
static esp_err_t register_set_options_sta_handler(void)
{
    ESP_LOGI(TAG, "Registering set options sta handler");
    const httpd_uri_t set_options_sta = {
            .uri       = "/set_options/sta",
            .method    = HTTP_POST,
            .handler   = set_options_sta_handler,
    };
    return httpd_register_uri_handler(server, &set_options_sta);
}

/**
 * @brief Registers a handler to publish an event to initialize the station mode module.
 * @param context Pointer to the event loop handle to publish event to. 
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
static esp_err_t register_init_sta_handler(esp_event_loop_handle_t* context)
{
    ESP_LOGI(TAG, "Registering init sta handler");
    const httpd_uri_t init_sta = {
            .uri       = "/init/sta",
            .method    = HTTP_POST,
            .handler   = init_sta_handler,
            .user_ctx  = (void*)context,
    };
    return httpd_register_uri_handler(server, &init_sta);
}

/**
 * @brief Registers a handler to set options for the MQTT client module.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
static esp_err_t register_set_options_mqtt_handler(void)
{
    ESP_LOGI(TAG, "Registering set options mqtt handler");
    const httpd_uri_t set_options_mqtt = {
            .uri       = "/set_options/mqtt",
            .method    = HTTP_POST,
            .handler   = set_options_mqtt_handler,
    };
    return httpd_register_uri_handler(server, &set_options_mqtt);
}

/**
 * @brief Registers a handler to publish an event to initialize the MQTT client module.
 * @param context Pointer to the the event loop handle to publish the event to.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
static esp_err_t register_init_mqtt_handler(esp_event_loop_handle_t* context)
{
    ESP_LOGI(TAG, "Registering init mqtt handler");
    const httpd_uri_t init_mqtt = {
            .uri       = "/init/mqtt",
            .method    = HTTP_POST,
            .handler   = init_mqtt_handler,
            .user_ctx  = (void*)context,
    };
    return httpd_register_uri_handler(server, &init_mqtt);
}

/**
 * @brief Registers a handler to download an update, writes it to the next partition, 
 *        sets it as the boot partition, and publishes an event that the partition is ready to boot.
 * @param context Pointer to the the event loop handle to publish the event to.
 * @return esp_err_t
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
static esp_err_t register_ota_update_handler(esp_event_loop_handle_t* context)
{
    ESP_LOGI(TAG, "Registering ota update handler");
    const httpd_uri_t ota_update = {
            .uri       = "/ota_update",
            .method    = HTTP_POST,
            .handler   = ota_update_handler,
            .user_ctx  = (void*)context,
    };
    return httpd_register_uri_handler(server, &ota_update);
}

/**
 * @brief Registers a handler to restart the esp.
 * @param context Pointer to the the event loop handle to publish the event to.
 * @return esp_err_t
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
static esp_err_t register_restart_handler(esp_event_loop_handle_t* context)
{
    ESP_LOGI(TAG, "Registering restart handler");
    const httpd_uri_t restart = {
            .uri       = "/restart",
            .method    = HTTP_POST,
            .handler   = restart_handler,
            .user_ctx  = (void*)context,
    };
    return httpd_register_uri_handler(server, &restart);
}

static esp_err_t register_index(void)
{
    ESP_LOGI(TAG, "Registering index handler");
    const httpd_uri_t index = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
    };
    return httpd_register_uri_handler(server, &index);
}

esp_err_t aug_http_start(esp_event_loop_handle_t* context)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    AUG_RETURN_CHECK(httpd_start(&server, &config));
    
    AUG_RETURN_CHECK(register_set_options_sta_handler());
    AUG_RETURN_CHECK(register_init_sta_handler(context));
    AUG_RETURN_CHECK(register_set_options_mqtt_handler());
    AUG_RETURN_CHECK(register_init_mqtt_handler(context));
    AUG_RETURN_CHECK(register_ota_update_handler(context));
    AUG_RETURN_CHECK(register_restart_handler(context));
    AUG_RETURN_CHECK(register_index());
    return ESP_OK;
}

esp_err_t aug_http_stop_webserver(void)
{
    ESP_LOGI(TAG, "Stopping server");
    AUG_RETURN_CHECK(httpd_stop(server));
    server = NULL;
    return ESP_OK;
}

bool aug_http_is_init(void)
{
    if (server)
        return true;
    return false;
}