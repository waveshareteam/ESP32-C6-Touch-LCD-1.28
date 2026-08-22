#include "board_qmi8658.h"

#include <math.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define QMI8658_SENSOR_ADDR 0x6B
#define QMI8658_WHO_AM_I 0x00
#define QMI8658_CTRL1 0x02
#define QMI8658_CTRL2 0x03
#define QMI8658_CTRL3 0x04
#define QMI8658_CTRL7 0x08
#define QMI8658_STATUS0 0x2E
#define QMI8658_AX_L 0x35
#define QMI8658_RESET 0x60

static const char *TAG = "board_qmi8658";
static i2c_master_dev_handle_t qmi_dev;
static bool initialized;

static esp_err_t qmi8658_reg_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(qmi_dev, &reg_addr, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t qmi8658_reg_write(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    uint8_t buf[8];
    if (len > sizeof(buf) - 1)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    return i2c_master_transmit(qmi_dev, buf, len + 1, pdMS_TO_TICKS(100));
}

esp_err_t board_qmi8658_init(i2c_master_bus_handle_t bus_handle)
{
    uint8_t id = 0;

    if (initialized)
    {
        return ESP_OK;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMI8658_SENSOR_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus_handle, &dev_cfg, &qmi_dev), TAG, "QMI8658 add device failed");
    ESP_RETURN_ON_ERROR(qmi8658_reg_read(QMI8658_WHO_AM_I, &id, 1), TAG, "QMI8658 WHO_AM_I read failed");

    if (id != 0x05)
    {
        ESP_LOGW(TAG, "Unexpected QMI8658 WHO_AM_I: 0x%02x", id);
    }

    ESP_RETURN_ON_ERROR(qmi8658_reg_write(QMI8658_RESET, (uint8_t[]){0xb0}, 1), TAG, "QMI8658 reset failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(qmi8658_reg_write(QMI8658_CTRL1, (uint8_t[]){0x40}, 1), TAG, "QMI8658 CTRL1 failed");
    ESP_RETURN_ON_ERROR(qmi8658_reg_write(QMI8658_CTRL7, (uint8_t[]){0x03}, 1), TAG, "QMI8658 CTRL7 failed");
    ESP_RETURN_ON_ERROR(qmi8658_reg_write(QMI8658_CTRL2, (uint8_t[]){0x95}, 1), TAG, "QMI8658 CTRL2 failed");
    ESP_RETURN_ON_ERROR(qmi8658_reg_write(QMI8658_CTRL3, (uint8_t[]){0xd5}, 1), TAG, "QMI8658 CTRL3 failed");

    initialized = true;
    ESP_LOGI(TAG, "QMI8658 initialized");
    return ESP_OK;
}

bool board_qmi8658_read_data(board_qmi8658_data_t *data)
{
    uint8_t status = 0;
    int16_t raw[6] = {0};

    if (!initialized || !data)
    {
        return false;
    }

    if (qmi8658_reg_read(QMI8658_STATUS0, &status, 1) != ESP_OK || !(status & 0x03))
    {
        return false;
    }

    if (qmi8658_reg_read(QMI8658_AX_L, (uint8_t *)raw, sizeof(raw)) != ESP_OK)
    {
        return false;
    }

    data->acc_x = raw[0];
    data->acc_y = raw[1];
    data->acc_z = raw[2];
    data->gyr_x = raw[3];
    data->gyr_y = raw[4];
    data->gyr_z = raw[5];

    float ax = data->acc_x;
    float ay = data->acc_y;
    float az = data->acc_z;
    data->angle_x = atan2f(ax, sqrtf(ay * ay + az * az)) * 57.29578f;
    data->angle_y = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
    data->angle_z = atan2f(sqrtf(ax * ax + ay * ay), az) * 57.29578f;
    return true;
}
