#ifndef PCF85063A_H
#define PCF85063A_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCF85063A_LIBRARY_VERSION "1.0.0"

#define PCF85063A_ADDRESS 0x51
#define PCF85063A_YEAR_OFFSET (1970)

/* Register overview - control & status registers */
#define PCF85063A_RTC_CTRL_1_ADDR      (0x00) /*!< Control Register 1 Address */
#define PCF85063A_RTC_CTRL_2_ADDR      (0x01) /*!< Control Register 2 Address */
#define PCF85063A_RTC_OFFSET_ADDR      (0x02) /*!< Offset Register Address */
#define PCF85063A_RTC_RAM_BYTE_ADDR    (0x03) /*!< RAM Register Address */

/* Register overview - time & date registers */
#define PCF85063A_RTC_SECOND_ADDR      (0x04) /*!< Seconds Register Address */
#define PCF85063A_RTC_MINUTE_ADDR      (0x05) /*!< Minutes Register Address */
#define PCF85063A_RTC_HOUR_ADDR        (0x06) /*!< Hours Register Address */
#define PCF85063A_RTC_DAY_ADDR         (0x07) /*!< Day Register Address */
#define PCF85063A_RTC_WDAY_ADDR        (0x08) /*!< Weekday Register Address */
#define PCF85063A_RTC_MONTH_ADDR       (0x09) /*!< Month Register Address */
#define PCF85063A_RTC_YEAR_ADDR        (0x0A) /*!< Year Register Address; real year = 1970 + RTC register year */

/* Register overview - alarm registers */
#define PCF85063A_RTC_SECOND_ALARM     (0x0B) /*!< Alarm Seconds Register Address */
#define PCF85063A_RTC_MINUTE_ALARM     (0x0C) /*!< Alarm Minutes Register Address */
#define PCF85063A_RTC_HOUR_ALARM       (0x0D) /*!< Alarm Hours Register Address */
#define PCF85063A_RTC_DAY_ALARM        (0x0E) /*!< Alarm Day Register Address */
#define PCF85063A_RTC_WDAY_ALARM       (0x0F) /*!< Alarm Weekday Register Address */

/* Register overview - timer registers */
#define PCF85063A_RTC_TIMER_VAL        (0x10) /*!< Timer Value Register Address */
#define PCF85063A_RTC_TIMER_MODE       (0x11) /*!< Timer Mode Register Address */

/* PCF85063A_RTC_CTRL_1 register */
#define PCF85063A_RTC_CTRL_1_EXT_TEST  (0x80) /*!< External Test */
#define PCF85063A_RTC_CTRL_1_STOP      (0x20) /*!< 0: RTC clock runs, 1: RTC clock is stopped */
#define PCF85063A_RTC_CTRL_1_SR        (0x10) /*!< 0: no software reset, 1: initiate software reset */
#define PCF85063A_RTC_CTRL_1_CIE       (0x04) /*!< 0: correction interrupt disabled, 1: enabled */
#define PCF85063A_RTC_CTRL_1_CAP_SEL   (0x01) /*!< 0: 7 pF, 1: 12.5 pF */

/* PCF85063A_RTC_CTRL_2 register */
#define PCF85063A_RTC_CTRL_2_AIE       (0x80) /*!< Alarm Interrupt Enable */
#define PCF85063A_RTC_CTRL_2_AF        (0x40) /*!< Alarm Flag */
#define PCF85063A_RTC_CTRL_2_MI        (0x20) /*!< Minute Interrupt Enable */
#define PCF85063A_RTC_CTRL_2_HMI       (0x10) /*!< Half Minute Interrupt */
#define PCF85063A_RTC_CTRL_2_TF        (0x08) /*!< Timer Flag */

#define PCF85063A_RTC_OFFSET_MODE      (0x80) /*!< Offset Mode */

#define PCF85063A_RTC_TIMER_MODE_TE    (0x04) /*!< Timer Enable */
#define PCF85063A_RTC_TIMER_MODE_TIE   (0x02) /*!< Timer Interrupt Enable */
#define PCF85063A_RTC_TIMER_MODE_TI_TP (0x01) /*!< Timer Interrupt Mode */

/* Timer Clock Frequency Selection (TCF[1:0]) - bits 4:3 of Timer Mode register */
#define PCF85063A_RTC_TIMER_TCF_4096HZ (0x00) /*!< 4.096 kHz timer source clock */
#define PCF85063A_RTC_TIMER_TCF_64HZ   (0x08) /*!< 64 Hz timer source clock */
#define PCF85063A_RTC_TIMER_TCF_1HZ    (0x10) /*!< 1 Hz timer source clock */
#define PCF85063A_RTC_TIMER_TCF_1_60HZ (0x18) /*!< 1/60 Hz timer source clock */

#define PCF85063A_RTC_TIMER_TI_TP      (0x01) /*!< 1: pulse mode, 0: level mode */

#define PCF85063A_RTC_ALARM            (0x80) /*!< Set AEN_x registers */
#define PCF85063A_RTC_CTRL_1_DEFAULT   (0x00) /*!< Control_1 default value */
#define PCF85063A_RTC_CTRL_2_DEFAULT   (0x00) /*!< Control_2 default value */
#define PCF85063A_RTC_TIMER_FLAG       (0x08) /*!< Timer Flag */

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t dotw;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
} pcf85063a_datetime_t;

typedef struct {
  i2c_master_bus_handle_t bus_handle;
  i2c_master_dev_handle_t dev_handle;
} pcf85063a_dev_t;

/**
 * @brief Initialize a PCF85063A device on an existing I2C master bus.
 *
 * @param dev Device context to initialize.
 * @param bus_handle ESP-IDF I2C master bus handle.
 * @param i2c_addr 7-bit PCF85063A I2C address.
 * @return ESP_OK on success, or an esp_err_t code on failure.
 */
esp_err_t pcf85063a_init(pcf85063a_dev_t *dev, i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr);

/**
 * @brief Issue a software reset to the PCF85063A.
 */
esp_err_t pcf85063a_reset(pcf85063a_dev_t *dev);

/**
 * @brief Set RTC time fields.
 */
esp_err_t pcf85063a_set_time(pcf85063a_dev_t *dev, pcf85063a_datetime_t time);

/**
 * @brief Set RTC date fields.
 */
esp_err_t pcf85063a_set_date(pcf85063a_dev_t *dev, pcf85063a_datetime_t date);

/**
 * @brief Set RTC time and date fields.
 */
esp_err_t pcf85063a_set_time_date(pcf85063a_dev_t *dev, pcf85063a_datetime_t time);

/**
 * @brief Read current RTC time and date.
 */
esp_err_t pcf85063a_get_time_date(pcf85063a_dev_t *dev, pcf85063a_datetime_t *time);

/**
 * @brief Enable alarm interrupt and clear the alarm flag.
 */
esp_err_t pcf85063a_enable_alarm(pcf85063a_dev_t *dev);

/**
 * @brief Read the alarm flag bits.
 */
esp_err_t pcf85063a_get_alarm_flag(pcf85063a_dev_t *dev, uint8_t *value);

/**
 * @brief Set alarm time fields.
 */
esp_err_t pcf85063a_set_alarm(pcf85063a_dev_t *dev, pcf85063a_datetime_t time);

/**
 * @brief Read configured alarm time fields.
 */
esp_err_t pcf85063a_get_alarm(pcf85063a_dev_t *dev, pcf85063a_datetime_t *time);

/**
 * @brief Enable timer with specified parameters.
 */
esp_err_t pcf85063a_enable_timer(pcf85063a_dev_t *dev, uint8_t timer_value, uint8_t timer_mode);

/**
 * @brief Set timer countdown value.
 */
esp_err_t pcf85063a_set_timer_value(pcf85063a_dev_t *dev, uint8_t timer_value);

/**
 * @brief Set timer mode configuration.
 */
esp_err_t pcf85063a_set_timer_mode(pcf85063a_dev_t *dev, uint8_t timer_mode);

/**
 * @brief Read timer flag status.
 */
esp_err_t pcf85063a_get_timer_flag(pcf85063a_dev_t *dev, uint8_t *flag);

/**
 * @brief Clear timer flag.
 */
esp_err_t pcf85063a_clear_timer_flag(pcf85063a_dev_t *dev);

/**
 * @brief Disable timer.
 */
esp_err_t pcf85063a_disable_timer(pcf85063a_dev_t *dev);

/**
 * @brief Read current timer countdown value.
 */
esp_err_t pcf85063a_get_timer_value(pcf85063a_dev_t *dev, uint8_t *timer_value);

/**
 * @brief Set timer using the recommended disable/configure/enable sequence.
 */
esp_err_t pcf85063a_set_timer_safe(pcf85063a_dev_t *dev, uint8_t timer_value, uint8_t timer_mode);

/**
 * @brief Convert date and time to a fixed-width string.
 *
 * @param datetime_str Output buffer.
 * @param datetime_str_size Output buffer size in bytes.
 * @param time Date and time value to format.
 */
void pcf85063a_datetime_to_str(char *datetime_str, size_t datetime_str_size, pcf85063a_datetime_t time);

esp_err_t pcf85063a_write_register(pcf85063a_dev_t *dev, uint8_t *pdata, uint8_t length);

esp_err_t pcf85063a_read_register(pcf85063a_dev_t *dev, uint8_t reg, uint8_t *buffer, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif // PCF85063A_H
