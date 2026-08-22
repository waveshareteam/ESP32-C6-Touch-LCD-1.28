#include <stdio.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <new>
#include "i2c_bsp.h"

static const char *TAG = "I2C_BSP";

static const gpio_num_t I2C_SCL_GPIO = GPIO_NUM_8;
static const gpio_num_t I2C_SDA_GPIO = GPIO_NUM_18;
static const i2c_port_t I2C_MASTER_PORT = I2C_NUM_0;
static const uint32_t I2C_DATA_TIMEOUT_TICKS = pdMS_TO_TICKS(5000);
static const uint32_t I2C_DONE_TIMEOUT_TICKS = pdMS_TO_TICKS(1000);

static I2cMasterBus *i2c_master_bus = NULL;
static i2c_master_dev_handle_t i2c_device_handles[128] = {};

static esp_err_t I2C_master_GetDeviceHandle(uint8_t dev_addr, i2c_master_dev_handle_t *dev_handle) {
  if (dev_handle == NULL || dev_addr >= 0x80) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = I2C_master_Init();
  if (ret != ESP_OK) {
    return ret;
  }

  if (i2c_device_handles[dev_addr] == NULL) {
    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = dev_addr;
    dev_config.scl_speed_hz = 400000;

    ret = i2c_master_bus_add_device(I2C_master_GetBusHandle(), &dev_config, &i2c_device_handles[dev_addr]);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to add I2C device 0x%02x (error: %d)", dev_addr, ret);
      return ret;
    }
  }

  *dev_handle = i2c_device_handles[dev_addr];
  return ESP_OK;
}

esp_err_t I2C_master_Init(void) {
  if (i2c_master_bus == NULL) {
    i2c_master_bus = new (std::nothrow) I2cMasterBus(I2C_SCL_GPIO, I2C_SDA_GPIO, I2C_MASTER_PORT);
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

esp_err_t I2C_write_buff(uint8_t dev_addr, uint8_t reg, const uint8_t *buf, uint8_t len) {
  i2c_master_dev_handle_t dev_handle = NULL;
  esp_err_t ret = I2C_master_GetDeviceHandle(dev_addr, &dev_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  return i2c_master_bus->i2c_write_buff(dev_handle, reg, buf, len);
}

esp_err_t I2C_read_buff(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len) {
  i2c_master_dev_handle_t dev_handle = NULL;
  esp_err_t ret = I2C_master_GetDeviceHandle(dev_addr, &dev_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  return i2c_master_bus->i2c_read_buff(dev_handle, reg, buf, len);
}

I2cMasterBus::I2cMasterBus(gpio_num_t scl_pin, gpio_num_t sda_pin, i2c_port_t i2c_port) {
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
  if (user_i2c_handle != NULL) {
    i2c_del_master_bus(user_i2c_handle);
    user_i2c_handle = NULL;
  }
}

esp_err_t I2cMasterBus::i2c_write_buff(i2c_master_dev_handle_t dev_handle, int reg, const uint8_t *buf, uint8_t len) {
  if (!user_i2c_handle || !dev_handle || !buf || len == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = i2c_master_bus_wait_all_done(user_i2c_handle, I2C_DONE_TIMEOUT_TICKS);
  if (ret != ESP_OK) {
    return ret;
  }

  if (reg == -1) {
    ret = i2c_master_transmit(dev_handle, buf, len, I2C_DATA_TIMEOUT_TICKS);
  } else {
    if (len > UINT8_MAX - 1) {
      return ESP_ERR_INVALID_ARG;
    }

    uint8_t write_buf[UINT8_MAX + 1] = { 0 };
    write_buf[0] = (uint8_t)reg;
    for (uint8_t i = 0; i < len; i++) {
      write_buf[i + 1] = buf[i];
    }
    ret = i2c_master_transmit(dev_handle, write_buf, len + 1, I2C_DATA_TIMEOUT_TICKS);
  }
  return ret;
}

esp_err_t I2cMasterBus::i2c_master_write_read_dev(i2c_master_dev_handle_t dev_handle, const uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen) {
  if (!user_i2c_handle || !dev_handle || !writeBuf || writeLen == 0 || !readBuf || readLen == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = i2c_master_bus_wait_all_done(user_i2c_handle, I2C_DONE_TIMEOUT_TICKS);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = i2c_master_transmit_receive(dev_handle, writeBuf, writeLen, readBuf, readLen, I2C_DATA_TIMEOUT_TICKS);
  return ret;
}

esp_err_t I2cMasterBus::i2c_read_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
  if (!user_i2c_handle || !dev_handle || !buf || len == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t addr = 0;
  esp_err_t ret = i2c_master_bus_wait_all_done(user_i2c_handle, I2C_DONE_TIMEOUT_TICKS);
  if (ret != ESP_OK) {
    return ret;
  }

  if (reg == -1) {
    ret = i2c_master_receive(dev_handle, buf, len, I2C_DATA_TIMEOUT_TICKS);
  } else {
    addr = (uint8_t)reg;
    ret = i2c_master_transmit_receive(dev_handle, &addr, 1, buf, len, I2C_DATA_TIMEOUT_TICKS);
  }
  return ret;
}

i2c_master_bus_handle_t I2cMasterBus::Get_I2cBusHandle() const {
  return user_i2c_handle;
}

esp_err_t I2cMasterBus::GetInitResult() const {
  return init_result;
}
