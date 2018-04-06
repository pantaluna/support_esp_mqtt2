/*
 * PURPOSE: endurance testing of mqtt_publish()
 */
#include <sys/dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_spiffs.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_mqtt.h"
#include "mjd.h"
#include "mjd_wifi.h"

/*
 * FreeRTOS settings
 */
#define MYAPP_RTOS_TASK_STACK_SIZE_HUGE (32768)
#define MYAPP_RTOS_TASK_PRIORITY_NORMAL (RTOS_TASK_PRIORITY_NORMAL)

/*
 * Logging
 */
static const char TAG[] = "myapp";

/*
 * KConfig: WIFI
 */
char *WIFI_SSID = CONFIG_MY_WIFI_SSID;
char *WIFI_PASSWORD = CONFIG_MY_WIFI_PASSWORD;

/*
 * MQTT
 */
#define MY_MQTT_HOST "broker.shiftr.io"
#define MY_MQTT_PORT 1883
#define MY_MQTT_USER "try"
#define MY_MQTT_PASS "try"

/*#define MY_MQTT_HOST "192.168.0.95" // @important The DNS name "s3..." does not work on an MCU@HomeLAN because it returns the ISP's WAN IP and this IP is not whitelisted in Ubuntu UFW!
#define MY_MQTT_PORT 12430
#define MY_MQTT_USER "uuuuu"
#define MY_MQTT_PASS "ppppp"*/

#define MY_MQTT_BUFFER_SIZE  (4096)  // @suggested 256 @used 4096
#define MY_MQTT_TIMEOUT      (2000)  // @suggested 2000 @used 2000

static EventGroupHandle_t mqtt_event_group;
static const int MQTT_CONNECTED_BIT = BIT0;
static const int MQTT_DISCONNECTED_BIT = BIT1;

static void mqtt_status_callback(esp_mqtt_status_t status) {
    switch (status) {
    case ESP_MQTT_STATUS_CONNECTED:
        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        xEventGroupClearBits(mqtt_event_group, MQTT_DISCONNECTED_BIT);
        break;
    case ESP_MQTT_STATUS_DISCONNECTED: // @important This bitflag is not set when stopping mqtt (only when an active netconn is ABORTED)...
        xEventGroupSetBits(mqtt_event_group, MQTT_DISCONNECTED_BIT);
        xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    }
}

static void mqtt_message_callback(const char *topic, uint8_t *payload, size_t len) {
    ESP_LOGI(TAG, "(should not happen in this prj...) subscription message received: topic=%s => payload=%s (%d)", topic,
            payload, (int ) len);
}

/*
 * LOGGING SPIFFS
 *  @doc The partition is not overwritten when writing a new application to flash (it is in another sector of the Flash EEPROM)
 */
static FILE* _log_fp;
static const char SPIFFS_PARTITION[] = "myspiffs";
static const char SPIFFS_BASE_PATH[] = "/spiffs";
static const char LOG_PATH[] = "/spiffs/log.txt";

static esp_err_t _log_df() { // ~bash df (disk free)
    // @seq After esp_vfs_spiffs_register()
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval;
    size_t total_bytes = 0, used_bytes = 0;

    f_retval = esp_spiffs_info(NULL, &total_bytes, &used_bytes);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information err %i", f_retval);
        return f_retval;
    }
    ESP_LOGI(TAG, "Partition %s size: total %u, used %u (%u%%)", SPIFFS_PARTITION, total_bytes, used_bytes,
            100 * used_bytes / total_bytes);

    return ESP_OK;
}

static esp_err_t _log_ls() {
    // @seq After esp_vfs_spiffs_register()
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    char dashes[255 + 1] = "";
    memset(dashes, '-', 255);

    ESP_LOGI(TAG, "%s %s", "ls", SPIFFS_BASE_PATH);
    ESP_LOGI(TAG, "%10.10s  %20.20s  %50.50s", "Type", "Size", "Name");
    ESP_LOGI(TAG, "%.10s  %.20s  %.50s", dashes, dashes, dashes);

    struct stat st;
    DIR *dir;
    dir = opendir(SPIFFS_BASE_PATH);
    if (!dir) {
        ESP_LOGE(TAG, "Error opening directory");
        return ESP_FAIL;
    }

    char uri[255];
    struct dirent *direntry;
    char snippet[255];
    char line[255];

    while ((direntry = readdir(dir)) != NULL) {
        switch (direntry->d_type) {
        case DT_DIR:  // A directory.
            ESP_LOGI(TAG, "%10s  %20s  %50s\n", "DIR", "", direntry->d_name);
            break;
        case DT_REG:  // A regular file.
            strcpy(line, "");
            sprintf(snippet, "%10s  ", "FILE");
            strcat(line, snippet);
            sprintf(uri, "%s/%s", SPIFFS_BASE_PATH, direntry->d_name);
            if (stat(uri, &st) == 0) {
                sprintf(snippet, "%20ld  ", st.st_size);
            } else {
                sprintf(snippet, "%20s  ", "?");
            }
            strcat(line, snippet);
            sprintf(snippet, "%50.50s", direntry->d_name);
            strcat(line, snippet);
            ESP_LOGI(TAG, "%s", line)
            ;
            break;
        case DT_UNKNOWN:  // The type is unknown.
            ESP_LOGI(TAG, "%10sn", "UNKNOWN")
            ;
            break;
        default:
            ESP_LOGI(TAG, "%10s", "?????")
            ;
        }
    }
    closedir(dir);

    ESP_LOGI(TAG, "");

    return ESP_OK;
}


/*
 * INIT
 */

/*
 * TASK
 */
void main_task(void *pvParameter) {
    esp_err_t f_retval;

    mjd_log_memory_statistics();

    /*
     * SPIFFS Register
     */

    // Register the SPIFFS filesystem.
    //      @important It only formats the partition when it is not yet formatted (or already formatted but with a different partiton layout such as with the tool mkspiffs)
    esp_vfs_spiffs_conf_t conf = { .base_path = SPIFFS_BASE_PATH, .partition_label = NULL, .max_files = 5,
            .format_if_mount_failed = true };
    f_retval = esp_vfs_spiffs_register(&conf);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_spiffs_register() failed");
        vTaskDelete(NULL);
    }

    // remove file (just in case the MCU was rebooted in the middle of the programme).
    /*remove(LOG_PATH);*/

    //
    _log_df();
    _log_ls();

    /*
     * SPIFFS Create file
     */
    ESP_LOGI(TAG, "Create SPIFFS text file...");

    bool bretval;
    int iretval;

    struct stat buffer;
    int file_exists = stat(LOG_PATH,&buffer);
    if(file_exists == 0) {
        ESP_LOGI(TAG, "  Logfile exists, do not create it again (takes a long time)");
        // GOTO
        goto section_wifi;
    }

    _log_fp = fopen(LOG_PATH, "wt"); // w=write t=textmode
    if (_log_fp == NULL) {
        ESP_LOGE(TAG, "Cannot create/open logfile");
        vTaskDelete(NULL);
    }

    static const uint32_t WRITE_CACHE_CYCLE = 500;
    static uint32_t counter_write_cache = 0;

    char dashes[128 + 1] = "";
    memset(dashes, '-', 128);

    const uint32_t MAX_COUNTER_APPEND = 5000;
    ESP_LOGI(TAG, "  Adding %i lines...", MAX_COUNTER_APPEND);

    uint32_t counter_append = 1;
    while (counter_append <= MAX_COUNTER_APPEND) {
        // Write to SPIFFS
        printf(".");
        fflush(stdout);

        if (_log_fp == NULL) {
            ESP_LOGE(TAG, "ABORT. file handle _log_fp is NULL\n");
            vTaskDelete(NULL);
        }

        iretval = fprintf(_log_fp, "%u %s\n", counter_append, dashes);
        if (iretval < 0) {
            ESP_LOGE(TAG, "ABORT. failed fprintf() iretval %i", iretval);
            break; // EXIT
        }

        // Smart commit
        counter_write_cache++;
        if (counter_write_cache % WRITE_CACHE_CYCLE == 0) {
            ESP_LOGI(TAG, "fsync'ing (WRITE_CACHE_CYCLE=%u)", WRITE_CACHE_CYCLE);
            fsync(fileno(_log_fp));
        }

        counter_append++;

        // @question yield a little to give time to other tasks???
        vTaskDelay(RTOS_DELAY_10MILLISEC);
    }
    printf("\n");

    // Close log file
    fclose(_log_fp);
    _log_fp = NULL;

    //
    _log_df();
    _log_ls();
    mjd_log_memory_statistics();

    /*
     * WIFI
     */

    // LABEL
    section_wifi:;

    ESP_LOGI(TAG, "***SECTION: WIFI***");
    ESP_LOGI(TAG, "WIFI_SSID:     %s", WIFI_SSID);
    ESP_LOGI(TAG, "WIFI_PASSWORD: %s", WIFI_PASSWORD);

    mjd_wifi_sta_init(WIFI_SSID, WIFI_PASSWORD);

    /*
     * MQTT
     */
    ESP_LOGI(TAG, "***SECTION: MQTT***");

    mqtt_event_group = xEventGroupCreate();
    esp_mqtt_init(mqtt_status_callback, mqtt_message_callback, MY_MQTT_BUFFER_SIZE, MY_MQTT_TIMEOUT);

    mjd_wifi_sta_start();

    esp_mqtt_start(MY_MQTT_HOST, MY_MQTT_PORT, "support_esp_mqtt2", MY_MQTT_USER, MY_MQTT_PASS);
    xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

    static const int QOS_1 = 1;
    char topic[128] = "esp32registry/device/churchstreet/log";
    char payload[1024] = "";
    uint32_t cur_line;

    mjd_log_memory_statistics();

    while (1) {
        // Open log file
        ESP_LOGI(TAG, "open log file");
        _log_fp = fopen(LOG_PATH, "rt"); // r=read t=textmode
        if (_log_fp == NULL) {
            ESP_LOGE(TAG, "Cannot open logfile");
            // GOTO
            goto section_cleanup;
        }

        mjd_log_memory_statistics();

        cur_line = 0;
        while (fgets(payload, sizeof(payload), _log_fp) != NULL) {
            ++cur_line;
            ESP_LOGI(TAG, "MQTT: LOOP#%i", cur_line);

            /////ESP_LOGI(TAG, "  esp_mqtt_publish(): topic (%i) %s => payload (%i) %s\n", strlen(topic), topic, strlen(payload), payload);

            mjd_log_memory_statistics();

            bretval = esp_mqtt_publish((char *) topic, (uint8_t *) payload, strlen(payload), QOS_1, false);
            if (bretval == false) {
                ESP_LOGE(TAG, "  ABORT. esp_mqtt_publish() failed");
                // GOTO
                goto section_cleanup;
            }

            // @question yield a little to give time to other tasks?
            /////vTaskDelay(RTOS_DELAY_10MILLISEC);
        }

        // close file
        ESP_LOGI(TAG, "  Close file");
        iretval = fclose(_log_fp);
        _log_fp = NULL;
        if (iretval != 0) {
            ESP_LOGE(TAG, "Cannot close logfile");
            // GOTO
            goto section_cleanup;
        }
    }

    // LABEL
    section_cleanup:;

    esp_mqtt_stop();

    mjd_wifi_sta_disconnect_stop();

    /********************************************************************************
     * Task Delete
     * @doc Passing NULL will end the current task
     */
    vTaskDelete(NULL);
}

/*
 * MAIN
 */
void app_main() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    /* MY Init */
    ESP_LOGI(TAG, "@doc exec nvs_flash_init() - mandatory for Wifi to work later on");
    nvs_flash_init();
    ESP_LOGI(TAG, "@doc Wait X seconds after power-on (start logic analyzer, let peripherals become active");
    vTaskDelay(RTOS_DELAY_1SEC);

    mjd_log_memory_statistics();

    /**********
     * TASK:
     * @important For stability (RMT + Wifi): always use xTaskCreatePinnedToCore(APP_CPU_NUM) [Opposed to xTaskCreate()]
     */
    BaseType_t xReturned;
    xReturned = xTaskCreatePinnedToCore(&main_task, "main_task (name)", MYAPP_RTOS_TASK_STACK_SIZE_HUGE, NULL,
    MYAPP_RTOS_TASK_PRIORITY_NORMAL, NULL, APP_CPU_NUM);
    if (xReturned == pdPASS) {

        ESP_LOGI(TAG, "OK Task has been created, and is running right now");
    }

    /**********
     * END
     */
    ESP_LOGI(TAG, "END %s()", __FUNCTION__);
}

