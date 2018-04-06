/*
 *
 */
#ifndef __MJD_H__
#define __MJD_H__

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event_loop.h"
#include "esp_clk.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "esp_system.h"

#include "cJSON.h"
#include "driver/i2c.h"
#include "driver/rmt.h"
#include "mbedtls/base64.h"
#include "nvs_flash.h"
#include "soc/rmt_reg.h"
#include "soc/soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********
 *  Definitions for errors
 *  @deprecated Use esp_err_t instead of mjd_err_t
 *  @deprecated Use ESP_OK instead of MJD_OK
 *  @deprecated Use ESP_FAIL instead of MJD_ERROR
 */
/////typedef int32_t mjd_err_t;
/////#define MJD_OK     (0)
/////#define MJD_ERROR  (-1)
#define MJD_ERR_CHECKSUM            (0x101)
#define MJD_ERR_INVALID_ARG         (0x102)
#define MJD_ERR_INVALID_DATA        (0x103)
#define MJD_ERR_INVALID_RESPONSE    (0x104)
#define MJD_ERR_INVALID_STATE       (0x105)
#define MJD_ERR_NOT_FOUND           (0x106)
#define MJD_ERR_NOT_SUPPORTED       (0x107)
#define MJD_ERR_REGEXP              (0x108)
#define MJD_ERR_TIMEOUT             (0x109)
#define MJD_ERR_IO                  (0x110)

#define MJD_ERR_ESP_GPIO            (0x201)
#define MJD_ERR_ESP_I2C             (0x202)
#define MJD_ERR_ESP_RMT             (0x203)
#define MJD_ERR_ESP_RTOS            (0x204)
#define MJD_ERR_ESP_SNTP            (0x205)
#define MJD_ERR_ESP_WIFI            (0x206)

#define MJD_ERR_LWIP                (0x301)
#define MJD_ERR_NETCONN             (0x302)

/**********
 * C Language: utilities, stdlib, etc.
 */
/*
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**********
 * INTEGERS: BINARY SEARCH
 */
int mjd_compare_ints(const void * a, const void * b);

/**********
 * BYTES and BINARY REPRESENTATION
 */
uint8_t mjd_byte_to_bcd(uint8_t val);
uint8_t mjd_bcd_to_byte(uint8_t val);
esp_err_t mjd_byte_to_binary_string(uint8_t input_byte, char * output_string);

/**********
 * STRINGS
 */
bool mjd_string_starts_with(const char *str, const char *pre);
bool mjd_string_ends_with(const char *str, const char *post);

/**********
 * DATE TIME
 * @doc unsigned int (uint32_t on ESP32) Maximum value: 4294967295
 */
#define SECONDS_PER_DAY               86400
#define SECONDS_FROM_1970_TO_2000 946684800

uint32_t mjd_seconds_to_milliseconds(uint32_t seconds);
uint32_t mjd_seconds_to_microseconds(uint32_t seconds);
void mjd_log_time();
void mjd_set_timezone_utc();
void mjd_set_timezone_amsterdam();
void mjd_get_current_time_yyyymmddhhmmss(char *ptr_buffer);

/**********
 * RTOS
 */
#define RTOS_DELAY_0             (0)
#define RTOS_DELAY_1MILLISEC     (   1 / portTICK_PERIOD_MS)
#define RTOS_DELAY_10MILLISEC    (  10 / portTICK_PERIOD_MS)
#define RTOS_DELAY_50MILLISEC    (  50 / portTICK_PERIOD_MS)
#define RTOS_DELAY_100MILLISEC   ( 100 / portTICK_PERIOD_MS)
#define RTOS_DELAY_150MILLISEC   ( 150 / portTICK_PERIOD_MS)
#define RTOS_DELAY_250MILLISEC   ( 250 / portTICK_PERIOD_MS)
#define RTOS_DELAY_500MILLISEC   ( 500 / portTICK_PERIOD_MS)
#define RTOS_DELAY_1SEC          (1 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_2SEC          (2 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_3SEC          (3 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_5SEC          (5 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_10SEC         (10 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_15SEC         (15 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_30SEC         (30 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_1MINUTE       (1 * 60 * 1000 / portTICK_PERIOD_MS)
#define RTOS_DELAY_5MINUTES      (5 * 60 * 1000 / portTICK_PERIOD_MS)

#define RTOS_TASK_PRIORITY_NORMAL (5)

void mjd_rtos_wait_forever();

/**********
 * ESP32 SYSTEM
 */
void mjd_log_clanguage_details();
void mjd_log_chip_info();
void mjd_log_wakeup_details();
void mjd_log_memory_statistics();

/**********
 * ESP32: BOOT INFO, DEEP SLEEP and WAKE UP
 */
uint32_t mjd_increment_mcu_boot_count();
void mjd_log_mcu_boot_count();
uint32_t mjd_get_mcu_boot_count();
void mjd_log_wakeup_details();

/**********
 * ESP32: LED
 */
#define HUZZAH32_GPIO_NUM_LED (GPIO_NUM_13)
#define HUZZAH32_GPIO_BITPIN_LED (1ULL<<HUZZAH32_GPIO_NUM_LED)

typedef enum {
    LED_WIRING_TYPE_DEFAULT = 1, /*!< Default */
    LED_WIRING_TYPE_DIODE_TO_GND = 1, /*!< Resistor LED-DIODE=> GND (MCU Adafruit Huzzah32) */
    LED_WIRING_TYPE_DIODE_FROM_VCC = 2, /*!< LED-DIODE<= Resistor VCC (MCU Lolin32Lite) */
} mjd_led_wiring_type_t;

typedef struct {
    uint32_t is_initialized; /*!< Helper to know if an element was initialized, or not. Mark 1. */
    uint64_t gpio_num; /*!< GPIO num pin */
    mjd_led_wiring_type_t wiring_type; /*!< Wiring Type */
} mjd_led_config_t;

void mjd_led_config(const mjd_led_config_t *led_config);
void mjd_led_on(int gpio_nr);
void mjd_led_off(int gpio_nr);
void mjd_led_blink_times(int gpio_nr, int times);
void mjd_led_mark_error(int gpio_nr);

#ifdef __cplusplus
}
#endif

#endif /* __MJD_H__ */
