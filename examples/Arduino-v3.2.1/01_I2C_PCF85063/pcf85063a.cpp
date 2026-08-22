#include "pcf85063a.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "PCF85063A";
static const int PCF85063A_I2C_TIMEOUT_MS = 1000;

static uint8_t dec_to_bcd(uint8_t value);
static uint8_t bcd_to_dec(uint8_t value);

esp_err_t pcf85063a_init(pcf85063a_dev_t *dev, i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr) {
  if (!dev || !bus_handle) {
    return ESP_ERR_INVALID_ARG;
  }

  dev->bus_handle = bus_handle;

  i2c_device_config_t dev_config = {};
  dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_config.device_address = i2c_addr;
  dev_config.scl_speed_hz = 400000;
  dev_config.scl_wait_us = 0;
  dev_config.flags.disable_ack_check = false;
  esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev->dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add I2C device");
    return ret;
  }

  uint8_t buf[2] = { PCF85063A_RTC_CTRL_1_ADDR, PCF85063A_RTC_CTRL_1_DEFAULT | PCF85063A_RTC_CTRL_1_CAP_SEL };
  ret = pcf85063a_write_register(dev, buf, 2);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure Control_1 register");
    return ret;
  }

  ESP_LOGI(TAG, "PCF85063A initialized successfully");
  return ret;
}

esp_err_t pcf85063a_reset(pcf85063a_dev_t *dev) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buf[2] = { PCF85063A_RTC_CTRL_1_ADDR,
                     PCF85063A_RTC_CTRL_1_DEFAULT | PCF85063A_RTC_CTRL_1_CAP_SEL | PCF85063A_RTC_CTRL_1_SR };
  return pcf85063a_write_register(dev, buf, 2);
}

esp_err_t pcf85063a_set_time(pcf85063a_dev_t *dev, pcf85063a_datetime_t time) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buf[4] = { PCF85063A_RTC_SECOND_ADDR,
                     dec_to_bcd(time.sec),
                     dec_to_bcd(time.min),
                     dec_to_bcd(time.hour) };
  return pcf85063a_write_register(dev, buf, 4);
}

esp_err_t pcf85063a_set_date(pcf85063a_dev_t *dev, pcf85063a_datetime_t date) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }
  uint8_t buf[5] = { PCF85063A_RTC_DAY_ADDR,
                     dec_to_bcd(date.day),
                     dec_to_bcd(date.dotw),
                     dec_to_bcd(date.month),
                     dec_to_bcd((uint8_t)(date.year - PCF85063A_YEAR_OFFSET)) };
  return pcf85063a_write_register(dev, buf, 5);
}

esp_err_t pcf85063a_set_time_date(pcf85063a_dev_t *dev, pcf85063a_datetime_t time) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }
  uint8_t buf[8] = { PCF85063A_RTC_SECOND_ADDR,
                     dec_to_bcd(time.sec),
                     dec_to_bcd(time.min),
                     dec_to_bcd(time.hour),
                     dec_to_bcd(time.day),
                     dec_to_bcd(time.dotw),
                     dec_to_bcd(time.month),
                     dec_to_bcd((uint8_t)(time.year - PCF85063A_YEAR_OFFSET)) };
  return pcf85063a_write_register(dev, buf, 8);
}

esp_err_t pcf85063a_get_time_date(pcf85063a_dev_t *dev, pcf85063a_datetime_t *time) {
  if (!dev || !time) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;
  uint8_t bufss[7] = { 0 };

  ret = pcf85063a_read_register(dev, PCF85063A_RTC_SECOND_ADDR, bufss, 7);
  if (ret != ESP_OK) {
    return ret;
  }
  time->sec = bcd_to_dec(bufss[0] & 0x7F);
  time->min = bcd_to_dec(bufss[1] & 0x7F);
  time->hour = bcd_to_dec(bufss[2] & 0x3F);
  time->day = bcd_to_dec(bufss[3] & 0x3F);
  time->dotw = bcd_to_dec(bufss[4] & 0x07);
  time->month = bcd_to_dec(bufss[5] & 0x1F);
  time->year = bcd_to_dec(bufss[6]) + PCF85063A_YEAR_OFFSET;

  return ret;
}

esp_err_t pcf85063a_enable_alarm(pcf85063a_dev_t *dev) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buf[2] = { PCF85063A_RTC_CTRL_2_ADDR,
                     PCF85063A_RTC_CTRL_2_DEFAULT | PCF85063A_RTC_CTRL_2_AIE };
  buf[1] &= ~PCF85063A_RTC_CTRL_2_AF;
  return pcf85063a_write_register(dev, buf, 2);
}

esp_err_t pcf85063a_get_alarm_flag(pcf85063a_dev_t *dev, uint8_t *value) {
  if (!dev || !value) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;

  ret = pcf85063a_read_register(dev, PCF85063A_RTC_CTRL_2_ADDR, value, 1);
  if (ret != ESP_OK) {
    return ret;
  }
  *value &= PCF85063A_RTC_CTRL_2_AF | PCF85063A_RTC_CTRL_2_AIE;

  return ret;
}

esp_err_t pcf85063a_set_alarm(pcf85063a_dev_t *dev, pcf85063a_datetime_t time) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buf[6] = { PCF85063A_RTC_SECOND_ALARM,
                     (uint8_t)(dec_to_bcd(time.sec) & 0x7F),
                     (uint8_t)(dec_to_bcd(time.min) & 0x7F),
                     (uint8_t)(dec_to_bcd(time.hour) & 0x7F),
                     PCF85063A_RTC_ALARM,    // Disable day
                     PCF85063A_RTC_ALARM };  // Disable weekday
  return pcf85063a_write_register(dev, buf, 6);
}

esp_err_t pcf85063a_get_alarm(pcf85063a_dev_t *dev, pcf85063a_datetime_t *time) {
  if (!dev || !time) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;
  uint8_t bufss[5] = { 0 };

  ret = pcf85063a_read_register(dev, PCF85063A_RTC_SECOND_ALARM, bufss, 5);
  if (ret != ESP_OK) {
    return ret;
  }
  time->sec = bcd_to_dec(bufss[0] & 0x7F);
  time->min = bcd_to_dec(bufss[1] & 0x7F);
  time->hour = bcd_to_dec(bufss[2] & 0x3F);
  time->day = bcd_to_dec(bufss[3] & 0x3F);
  time->dotw = bcd_to_dec(bufss[4] & 0x07);

  return ret;
}

esp_err_t pcf85063a_enable_timer(pcf85063a_dev_t *dev, uint8_t timer_value, uint8_t timer_mode) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  // Timer value 0 stops the timer according to datasheet
  if (timer_value == 0) {
    return pcf85063a_disable_timer(dev);
  }

  // Use safe configuration sequence
  return pcf85063a_set_timer_safe(dev, timer_value, timer_mode);
}

esp_err_t pcf85063a_set_timer_value(pcf85063a_dev_t *dev, uint8_t timer_value) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buf[2] = { PCF85063A_RTC_TIMER_VAL, timer_value };
  return pcf85063a_write_register(dev, buf, 2);
}

esp_err_t pcf85063a_set_timer_mode(pcf85063a_dev_t *dev, uint8_t timer_mode) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buf[2] = { PCF85063A_RTC_TIMER_MODE, timer_mode };
  return pcf85063a_write_register(dev, buf, 2);
}

esp_err_t pcf85063a_get_timer_flag(pcf85063a_dev_t *dev, uint8_t *flag) {
  if (!dev || !flag) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;

  ret = pcf85063a_read_register(dev, PCF85063A_RTC_CTRL_2_ADDR, flag, 1);
  if (ret != ESP_OK) {
    return ret;
  }
  *flag &= PCF85063A_RTC_CTRL_2_TF;  // Mask to get only the timer flag bit

  return ret;
}

esp_err_t pcf85063a_clear_timer_flag(pcf85063a_dev_t *dev) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;
  uint8_t ctrl2_val;

  // Read current Control_2 register value
  ret = pcf85063a_read_register(dev, PCF85063A_RTC_CTRL_2_ADDR, &ctrl2_val, 1);
  if (ret != ESP_OK) {
    return ret;
  }

  // Clear timer flag bit (write 0 to clear, preserve other bits)
  ctrl2_val &= ~PCF85063A_RTC_CTRL_2_TF;

  uint8_t buf[2] = { PCF85063A_RTC_CTRL_2_ADDR, ctrl2_val };
  return pcf85063a_write_register(dev, buf, 2);
}

esp_err_t pcf85063a_disable_timer(pcf85063a_dev_t *dev) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;
  uint8_t timer_mode;

  // Read current timer mode
  ret = pcf85063a_read_register(dev, PCF85063A_RTC_TIMER_MODE, &timer_mode, 1);
  if (ret != ESP_OK) {
    return ret;
  }

  // Disable timer by clearing TE bit
  timer_mode &= ~PCF85063A_RTC_TIMER_MODE_TE;

  uint8_t buf[2] = { PCF85063A_RTC_TIMER_MODE, timer_mode };
  return pcf85063a_write_register(dev, buf, 2);
}

esp_err_t pcf85063a_get_timer_value(pcf85063a_dev_t *dev, uint8_t *timer_value) {
  if (!dev || !timer_value) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;
  uint8_t value = 0;

  ret = pcf85063a_read_register(dev, PCF85063A_RTC_TIMER_VAL, &value, 1);
  if (ret != ESP_OK) {
    return ret;
  }

  *timer_value = value;

  return ESP_OK;
}

esp_err_t pcf85063a_set_timer_safe(pcf85063a_dev_t *dev, uint8_t timer_value, uint8_t timer_mode) {
  if (!dev) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;

  // Disable timer first as recommended by datasheet
  ret = pcf85063a_disable_timer(dev);
  if (ret != ESP_OK) {
    return ret;
  }

  // Set timer value
  ret = pcf85063a_set_timer_value(dev, timer_value);
  if (ret != ESP_OK) {
    return ret;
  }

  // Set timer mode and enable
  ret = pcf85063a_set_timer_mode(dev, timer_mode | PCF85063A_RTC_TIMER_MODE_TE);

  return ret;
}

/**
 * Convert normal decimal numbers to binary coded decimal 
 **/
static uint8_t dec_to_bcd(uint8_t value) {
  return (uint8_t)((value / 10 * 16) + (value % 10));
}

/**
 * Convert binary coded decimal to normal decimal numbers  
 **/
static uint8_t bcd_to_dec(uint8_t value) {
  return (uint8_t)((value / 16 * 10) + (value % 16));
}

/**
 * Convert time to string 
 **/
void pcf85063a_datetime_to_str(char *datetime_str, size_t datetime_str_size, pcf85063a_datetime_t time) {
  if (!datetime_str || datetime_str_size == 0) {
    return;
  }

  snprintf(datetime_str, datetime_str_size, "%04u/%02u/%02u %02u:%02u:%02u",
           (unsigned int)time.year,
           (unsigned int)time.month,
           (unsigned int)time.day,
           (unsigned int)time.hour,
           (unsigned int)time.min,
           (unsigned int)time.sec);
}

esp_err_t pcf85063a_write_register(pcf85063a_dev_t *dev, uint8_t *pdata, uint8_t length) {
  if (!dev || !dev->dev_handle || !pdata || length == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  return i2c_master_transmit(dev->dev_handle, pdata, length, PCF85063A_I2C_TIMEOUT_MS);
}

esp_err_t pcf85063a_read_register(pcf85063a_dev_t *dev, uint8_t reg, uint8_t *buffer, uint8_t length) {
  if (!dev || !buffer || length == 0 || !dev->dev_handle) {
    return ESP_ERR_INVALID_ARG;
  }

  return i2c_master_transmit_receive(dev->dev_handle, &reg, 1, buffer, length, PCF85063A_I2C_TIMEOUT_MS);
}
