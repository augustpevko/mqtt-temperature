# MQTT Temperature

This application combines an HTTP server, a DS18B20 temperature sensor module, and an MQTT client to provide temperature readings via MQTT. It's developed using the [IDF (ESP32 IoT Development Framework)](https://github.com/espressif/esp-idf).

## Configure the Project
**Set up the ESP-IDF environment**: Make sure you have the ESP-IDF installed and configured. Refer to the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for instructions.

**Clone the repository**: Clone the repository containing the MQTT Temperature application:
    
    git clone https://github.com/mqtt-temperature.git
    cd mqtt-temperature

**Open the project configuration menu:**
    
    idf.py menuconfig

### Project Configuration

**STA Mode Settings:**
- Provide info about wifi
    - Set `WiFi SSID`
    - Set `WiFi Password`
    > Note: You can choose not to provide information about your access point in the `Project configuration`. In this case, the ESP will initialize access point mode, and you will be able to configure it via an HTTP server.

**AP Mode Settings:**
- Set `WiFi SSID`
- Set `WiFi Password`

**MQTT Settings:**
- Set `Broker URI`

**DS18B20 Settings:**
- Set `DS18B20 GPIO`

`sdkconfig` contains minimal system settings without which the ESP can't run normally:

- `ESP_MAIN_TASK_STACK_SIZE` from `3584` (default value) to `4096`. Stack overflow may happen if there are many large buffers on the stack.
- `HTTPD_MAX_REQ_HDR_LEN` from `512` (default value) to `1024`. Some browsers may have long header fields, causing errors.
- `PARTITION_TABLE_TWO_OTA` from `n` (default value) to `y` for OTA updates.
- `ESPTOOLPY_FLASHSIZE` from `2MB` (default value) to `4MB` to flash the application.

## Build and Flash

Build the project and flash it to the board, then run the monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type `Ctrl-]`.)

Project supports OTA via HTTP server (see HTTP endpoints).

## HTTP Endpoints

**GET /**:
- Returns `index.html`.

**POST /init/sta**:
- Initialize station mode with current options.

**POST /init/mqtt**:
- Connects to MQTT broker with current options.

**POST /set_options/sta**:
- Takes settings from the query string and assigns it to station mode configuration. Query string should have the following keys:
    - `ssid`: access point SSID (network name).
    - `password`: access point password (WPA or WPA2).
    - `max_retry`: max retries to connect to access point.
    - `auth_mode`: Wi-Fi authentication mode threshold.
    - `sae_mode`: select mode for SAE as Hunt and Peck, H2E, or both.
    - `password_id`: password identifier for SAE H2E.

**POST /set_options/mqtt**:
- Takes settings from the query string and assigns it to MQTT client configuration. Query string should have the following keys:
    - `uri`: URI of the broker to connect to.

**POST /ota_update**:
- Takes firmware binary file, writes it to the boot partition and reboots.

### Curl Usage Examples
```
curl -X POST "http://espserver/init/sta"
```
```
curl -X POST "http://espserver/init/mqtt"
```
```
curl -X POST "http://espserver/set_options/sta?ssid=myssid&password=mypassword&max_retry=5&auth_mode=wpa2_psk&sae_mode=both&password_id="
```
```
curl -X POST "http://espserver/set_options/mqtt?uri=mqtt://mqtt.eclipseprojects.io"
```
```
curl --progress-bar -X POST --data-binary @build/mqtt_temperature.bin "http://espserver/ota_update" | tee /dev/null
```

## Example Output

```
I (678) aug wifi: Initializing wifi needed resources
I (688) aug wifi sta mode: Initializing wifi-STA resources
I (698) wifi:wifi driver task: 3ffc25d0, prio:23, stack:6656, core=0
I (728) wifi:wifi firmware version: cc1dd81
I (728) wifi:wifi certification version: v7.0
I (728) wifi:config NVS flash: enabled
I (728) wifi:config nano formating: disabled
I (738) wifi:Init data frame dynamic rx buffer num: 32
I (738) wifi:Init static rx mgmt buffer num: 5
I (748) wifi:Init management short buffer num: 32
I (748) wifi:Init dynamic tx buffer num: 32
I (748) wifi:Init static rx buffer size: 1600
I (758) wifi:Init static rx buffer num: 10
I (758) wifi:Init dynamic rx buffer num: 32
I (768) wifi_init: rx ba win: 6
I (768) wifi_init: tcpip mbox: 32
I (768) wifi_init: udp mbox: 6
I (778) wifi_init: tcp mbox: 6
I (778) wifi_init: tcp tx win: 5760
I (788) wifi_init: tcp rx win: 5760
I (788) wifi_init: tcp mss: 1440
I (788) wifi_init: WiFi IRAM OP enabled
I (798) wifi_init: WiFi RX IRAM OP enabled
I (808) phy_init: phy_version 4791,2c4672b,Dec 20 2023,16:06:06
I (888) wifi:mode : sta (d4:8a:fc:c7:12:28)
I (888) wifi:enable tsf
I (888) aug wifi sta mode: Waiting for connection
I (888) aug wifi sta mode: Trying to connect to the AP...
I (908) wifi:new:<11,2>, old:<1,0>, ap:<255,255>, sta:<11,2>, prof:1
I (908) wifi:state: init -> auth (b0)
I (918) wifi:state: auth -> assoc (0)
I (918) wifi:state: assoc -> run (10)
I (938) wifi:connected with testssid, aid = 4, channel 11, 40D, bssid = 13:cc:19:09:bc:5a
I (938) wifi:security: WPA2-PSK, phy: bgn, rssi: -80
I (938) wifi:pm start, type: 1

I (938) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (998) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (6948) esp_netif_handlers: sta ip: 192.168.0.115, mask: 255.255.255.0, gw: 192.168.0.1
I (6948) aug wifi sta mode: Got ip:192.168.0.115
I (6948) aug wifi sta mode: Connected to ap SSID:testssid password:testpassword
I (6958) DS18B20S: initializing DS18B20
I (6968) gpio: GPIO[23]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (6968) gpio: GPIO[23]| InputEn: 1| OutputEn: 1| OpenDrain: 1| Pullup: 1| Pulldown: 0| Intr:0
I (6978) DS18B20S: 1-Wire bus installed on GPIO23
I (6988) DS18B20S: Device iterator created, start searching...
I (7108) DS18B20S: Found a DS18B20[0], address: 5A031761DE3DFF28
I (7108) DS18B20S: Max DS18B20 number reached, stop searching...
I (7108) DS18B20S: Searching done, 1 DS18B20 device(s) found
I (7118) http server: Starting server on port: '80'
I (7128) http server: Registering set options sta handler
I (7128) http server: Registering init sta handler
I (7128) http server: Registering set options mqtt handler
I (7138) http server: Registering init mqtt handler
I (7148) http server: Registering ota update handler
I (7148) http server: Registering restart handler
I (7158) http server: Registering index handler
I (7158) mqtt client: Initializing mqtt client
I (7168) mqtt client: Other event id:7
I (7358) wifi:<ba-add>idx:0 (ifx:0, 13:cc:19:09:bc:5a), tid:0, ssn:0, winSize:64
I (7398) mqtt client: MQTT_EVENT_CONNECTED
I (37988) mqtt client: Sent publish successful, msg_id=0, topic=/devices/rtl-esp-wroom44[1]/controls/temperature/meta/type, data=temperature
I (37998) mqtt client: Sent publish successful, msg_id=0, topic=/devices/rtl-esp-wroom44[1]/controls/temperature/meta/readonly, data=1
I (38008) mqtt client: Sent publish successful, msg_id=0, topic=/devices/rtl-esp-wroom44[1]/controls/temperature, data=27.19
```
