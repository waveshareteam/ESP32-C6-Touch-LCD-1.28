#include <stdio.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <new>
#include "i2c_bsp.h"

static const char *TAG = "I2C_BSP";

static uint32_t i2c_data_timeout_ticks = 0;
static uint32_t i2c_done_timeout_ticks = 0;
static I2cMasterBus *i2c_master_bus = NULL;

esp_err_t I2C_master_Init(void) {
  if (i2c_master_bus == NULL) {
    i2c_master_bus = new (std::nothrow) I2cMasterBus(GPIO_NUM_8, GPIO_NUM_18, I2C_NUM_0);
    if (i2c_master_bus == NULL) {
      ESP_LOGE(TAG, "Failed to allocate I2C master bus");
      return ESP_ERR_NO_MEM;
    }
  }

  return i2c_master_bus->GetInitResult();
}

i2c_master_bus_handle_t I2C_master_GetBusHandle(void) {
  return i2c_master_bus ? i2c_master_bus->Get_I2cBusHandle() : NULL;
}

I2cMasterBus::I2cMasterBus(gpio_num_t scl_pin, gpio_num_t sda_pin, i2c_port_t i2c_port) {
  i2c_data_timeout_ticks = pdMS_TO_TICKS(5000);
  i2c_done_timeout_ticks = pdMS_TO_TICKS(1000);

  i2c_master_bus_config_t i2c_bus_config = {};
  i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
  i2c_bus_config.i2c_port = i2c_port;
  i2c_bus_config.scl_io_num = scl_pin;
  i2c_bus_config.sda_io_num = sda_pin;
  i2c_bus_config.glitch_ignore_cnt = 7;
  i2c_bus_config.flags.enable_internal_pullup = true;
  init_result = i2c_new_master_bus(&i2c_bus_config, &user_i2c_handle);
  if (init_result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create I2C master bus (error: %d)", init_result);
  }
}

I2cMasterBus::~I2cMasterBus() {
}

esp_err_t I2cMasterBus::i2c_write_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
  if (!user_i2c_handle || !dev_handle || !buf || len == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = i2c_master_bus_wait_all_done(user_i2c_handle, i2c_done_timeout_ticks);
  if (ret != ESP_OK) {
    return ret;
  }

  if (reg == -1) {
    ret = i2c_master_transmit(dev_handle, buf, len, i2c_data_timeout_ticks);
  } else {
    uint8_t write_buf[UINT8_MAX + 1] = { 0 };
    write_buf[0] = (uint8_t)reg;
    for (uint8_t i = 0; i < len; i++) {
      write_buf[i + 1] = buf[i];
    }
    ret = i2c_master_transmit(dev_handle, write_buf, len + 1, i2c_data_timeout_ticks);
  }
  return ret;
}

esp_err_t I2cMasterBus::i2c_master_write_read_dev(i2c_master_dev_handle_t dev_handle, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen) {
  if (!user_i2c_handle || !dev_handle || !writeBuf || writeLen == 0 || !readBuf || readLen == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = i2c_master_bus_wait_all_done(user_i2c_handle, i2c_done_timeout_ticks);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = i2c_master_transmit_receive(dev_handle, writeBuf, writeLen, readBuf, readLen, i2c_data_timeout_ticks);
  return ret;
}

esp_err_t I2cMasterBus::i2c_read_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
  if (!user_i2c_handle || !dev_handle || !buf || len == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t addr = 0;
  esp_err_t ret = i2c_master_bus_wait_all_done(user_i2c_handle, i2c_done_timeout_ticks);
  if (ret != ESP_OK) {
    return ret;
  }

  if (reg == -1) {
    ret = i2c_master_receive(dev_handle, buf, len, i2c_data_timeout_ticks);
  } else {
    addr = (uint8_t)reg;
    ret = i2c_master_transmit_receive(dev_handle, &addr, 1, buf, len, i2c_data_timeout_ticks);
  }
  return ret;
}

i2c_master_bus_handle_t I2cMasterBus::Get_I2cBusHandle() const {
  return user_i2c_handle;
}

esp_err_t I2cMasterBus::GetInitResult() const {
  return init_result;
}
