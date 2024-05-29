idf_component_register(SRCS "aug_nvs.c" "aug_utility.c" "aug_ds18b20.c" "aug_mqtt_client.c" "aug_wifi.c" "aug_wifi_sta.c" "aug_wifi_scan.c" "aug_wifi_ap.c" "aug_http_server.c" "main.c"
                    INCLUDE_DIRS "./include"
                    EMBED_FILES "html/index.html")