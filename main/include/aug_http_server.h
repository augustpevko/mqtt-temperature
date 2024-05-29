/**
 * @file aug_http_server.h
 * @brief Initializes the HTTP server, sets station mode and MQTT client configurations,
 *        and publishes events to the event loop to handle them in other modules.
 *        It also receives a firmware file and writes it to the boot partition and publishes the event.
 * @todo Test all buffers that are being created in the source file.
 * @todo Fix the issue that HTTP server doesn't wait for 
 *       tasks to be finished.
 * @todo Better responds when something goes wrong. 
 */

#if !defined(AUG_HTTP_SERVER_H)
#define AUG_HTTP_SERVER_H
    
#include <stdint.h>
#include <stdbool.h>

#include <esp_http_server.h>
#include <esp_event.h>
#include <esp_check.h>

/**
 * @brief Constructs a new esp event declare base object
 *        for publishing HTTP server events.
 */
ESP_EVENT_DECLARE_BASE(AUG_HTTP_SERVER_EVENTS);
enum {
    /**
     * @brief Event publishes when the HTTP server receiving request to init station mode.
     */
    AUG_HTTP_SERVER_EVENT_INIT_STA,
    /**
     * @brief Event publishes when the HTTP server receiving request to init MQTT client.
     */
    AUG_HTTP_SERVER_EVENT_INIT_MQTT,
    /**
     * @brief Event publishes when the HTTP server receiving OTA update.
     */
    AUG_HTTP_SERVER_EVENT_OTA_UPDATE,
    /**
     * @brief Event publishes when the HTTP server receiving request to restart.
     */
    AUG_HTTP_SERVER_EVENT_RESTART,
};

/**
 * @brief Starts an HTTP server and registers the handlers.
 * Allocates resources that should be freed with aug_http_stop_webserver.
 * @param context Pointer to the event loop handle to publish event to. 
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_http_start(esp_event_loop_handle_t* context);
/**
 * @brief Stops an HTTP server.
 * Deallocates resources that was allocated with aug_http_start.
 * @return esp_err_t 
 *      - ESP_OK: succeed 
 *      - others: refer to error code esp_err.h
 */
esp_err_t aug_http_stop_webserver(void);
/**
 * @brief Returns the current state of this module.
 * @return true If the module is initialized.
 * @return false If the module is not initialized.
 */
bool aug_http_is_init(void);

#endif