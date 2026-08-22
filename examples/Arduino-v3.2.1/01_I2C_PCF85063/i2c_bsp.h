#ifndef I2C_BSP_H
#define I2C_BSP_H

#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

esp_err_t I2C_master_Init(void);
i2c_master_bus_handle_t I2C_master_GetBusHandle(void);

class I2cMasterBus {
private:
  i2c_master_bus_handle_t user_i2c_handle = NULL;
  esp_err_t init_result = ESP_FAIL;
public:
  I2cMasterBus(gpio_num_t scl_pin, gpio_num_t sda_pin, i2c_port_t i2c_port);
  ~I2cMasterBus();

  esp_err_t i2c_write_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len);
  esp_err_t i2c_master_write_read_dev(i2c_master_dev_handle_t dev_handle, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen);
  esp_err_t i2c_read_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len);
  i2c_master_bus_handle_t Get_I2cBusHandle() const;
  esp_err_t GetInitResult() const;
};

#endif
