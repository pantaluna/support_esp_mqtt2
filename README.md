https://github.com/pantaluna/support_esp_mqtt2
https://github.com/256dpi/esp-mqtt/

esp-mqtt
 
# 1. esp_mqtt_publish() sometimes returns err LWMQTT_MISSING_OR_WRONG_PACKET (-9) during an endurance test of publishing 1000-200000 messages.
The program creates a text file of 5000 lines on a SPIFFS partition, and then mqtt publishes each text line to a mqtt server, and repeats this until an error occurs.
The moment of the fatal error varies, it can be during sending the 100th message or the 50000th message or never; it is always a different moment.
The error is always the same: "esp_mqtt: lwmqtt_publish: -9". I think -9 means LWMQTT_MISSING_OR_WRONG_PACKET.

@tip Define your WIFI SSID & password using menuconfig.

MSYSS2 Commands:

```
cd ~
git clone --recursive https://github.com/pantaluna/support_esp_mqtt2.git
cd  support_esp_mqtt2
make menuconfig
make flash monitor
```

UART Output:

```
...
I (160724) myapp: MQTT: LOOP#1290
I (160764) myapp: MQTT: LOOP#1291
I (160824) myapp: MQTT: LOOP#1292
I (160864) myapp: MQTT: LOOP#1293
E (162874) esp_mqtt: lwmqtt_publish: -9
E (162884) myapp:   ABORT. esp_mqtt_publish() failed
```

```
I (144314) myapp: MQTT: LOOP#998
I (144434) myapp: MQTT: LOOP#999
I (144574) myapp: MQTT: LOOP#1000
I (144714) myapp: MQTT: LOOP#1001
I (144844) myapp: MQTT: LOOP#1002
E (146844) esp_mqtt: lwmqtt_publish: -9
```

# 2. Environment
## Hardware
- MCU: Adafruit HUZZAH32 ESP32 development board.
- MCU: Wemos LOLIN32 Lite development board.

## Software
- ESP-IDF Github master branch of Apr 21, 2018.
- esp-mqtt V.0.5.2

#.
# APPENDICES
#.

# LWIP error codes
typedef enum {
  LWMQTT_SUCCESS = 0,
  LWMQTT_BUFFER_TOO_SHORT = -1,
  LWMQTT_VARNUM_OVERFLOW = -2,
  LWMQTT_NETWORK_FAILED_CONNECT = -3,
  LWMQTT_NETWORK_TIMEOUT = -4,
  LWMQTT_NETWORK_FAILED_READ = -5,
  LWMQTT_NETWORK_FAILED_WRITE = -6,
  LWMQTT_REMAINING_LENGTH_OVERFLOW = -7,
  LWMQTT_REMAINING_LENGTH_MISMATCH = -8,
  LWMQTT_MISSING_OR_WRONG_PACKET = -9,
  LWMQTT_CONNECTION_DENIED = -10,
  LWMQTT_FAILED_SUBSCRIPTION = -11,
  LWMQTT_SUBACK_ARRAY_OVERFLOW = -12,
  LWMQTT_PONG_TIMEOUT = -13,
} lwmqtt_err_t;


# Get esp-mqtt from Github

### INITIAL CLONE

#MINGW>>
mkdir --parents /c/myiot/esp/support_esp_mqtt2/components
cd              /c/myiot/esp/support_esp_mqtt2/components

git clone --recursive https://github.com/256dpi/esp-mqtt.git
cd esp-mqtt
git status

### Checkout tag v0.5.4
cd  /c/myiot/esp/support_esp_mqtt2/components/esp-mqtt
git pull
git submodule update --init --recursive
git status

git tag --list
git checkout tags/v0.5.4
git describe --tags
git submodule update --init --recursive
git status

### [UNSTABLE] Checkout branch idf3
cd  /c/myiot/esp/support_esp_mqtt2/components/esp-mqtt
git pull
git submodule update --init --recursive
git status

git branch
git checkout idf3
git show-branch
git submodule update --init --recursive
git status


# SOP upload to GITHUB
https://github.com/pantaluna/support_esp_mqtt2

## a. BROWSER: create github public repo support_esp_mqtt2 of pantaluna at Github.com

## b. MSYS2 git
```
git config --system --unset credential.helper
git config credential.helper store
git config --list

cd  /c/myiot/esp/support_esp_mqtt2
git init
git add .
git commit -m "First commit"
git remote add origin https://github.com/pantaluna/support_esp_mqtt2.git
git push --set-upstream origin master

git remote show origin

git tag --annotate v1.0 --message "The original bug report"
git push origin --tags
git status

```

# 2. SOP update repo
```
cd  /c/myiot/esp/support_esp_mqtt2
git add .
git commit -m "Another commit"
git push --set-upstream origin master

git status
```
