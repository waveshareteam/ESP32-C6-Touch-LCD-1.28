#include "board_pcf85063.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PCF85063_DEVICE_ADDR 0x51
#define PCF85063_SECONDS 0x04

static const char *TAG = "board_pcf85063";
static i2c_master_dev_handle_t rtc_dev;
static bool initialized;

static uint8_t dec2bcd(uint8_t value)
{
    return ((value / 10) << 4) + (value % 10);
}

static uint8_t bcd2dec(uint8_t value)
{
    return (((value & 0xF0) >> 4) * 10) + (value & 0x0F);
}

static esp_err_t pcf85063_reg_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(rtc_dev, &reg_addr, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t pcf85063_reg_write(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    uint8_t buf[8];
    if (len > sizeof(buf) - 1)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    return i2c_master_transmit(rtc_dev, buf, len + 1, pdMS_TO_TICKS(100));
}

esp_err_t board_pcf85063_init(i2c_master_bus_handle_t bus_handle)
{
    uint8_t seconds = 0;

    if (initialized)
    {
        return ESP_OK;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCF85063_DEVICE_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus_handle, &dev_cfg, &rtc_dev), TAG, "PCF85063 add device failed");
    ESP_RETURN_ON_ERROR(pcf85063_reg_read(PCF85063_SECONDS, &seconds, 1), TAG, "PCF85063 read failed");

    initialized = true;
    if (seconds & 0x80)
    {
        ESP_LOGW(TAG, "RTC oscillator stop detected");
    }
    else
    {
        ESP_LOGI(TAG, "PCF85063 initialized");
    }

    struct tm now_tm = {0};
    if (board_pcf85063_get_time(&now_tm) && (now_tm.tm_year < 125 || now_tm.tm_year > 130))
    {
        now_tm.tm_year = 2026 - 1900;
        now_tm.tm_mon = 6 - 1;
        now_tm.tm_mday = 15;
        now_tm.tm_hour = 12;
        now_tm.tm_min = 0;
        now_tm.tm_sec = 0;
        now_tm.tm_isdst = -1;
        board_pcf85063_set_time(&now_tm);
    }

    return ESP_OK;
}

bool board_pcf85063_get_time(struct tm *now_tm)
{
    uint8_t time_data[7] = {0};

    if (!initialized || !now_tm || pcf85063_reg_read(PCF85063_SECONDS, time_data, sizeof(time_data)) != ESP_OK)
    {
        return false;
    }

    now_tm->tm_sec = bcd2dec(time_data[0] & 0x7F);
    now_tm->tm_min = bcd2dec(time_data[1] & 0x7F);
    now_tm->tm_hour = bcd2dec(time_data[2] & 0x3F);
    now_tm->tm_mday = bcd2dec(time_data[3] & 0x3F);
    now_tm->tm_wday = bcd2dec(time_data[4] & 0x07);
    now_tm->tm_mon = bcd2dec(time_data[5] & 0x1F) - 1;
    now_tm->tm_year = bcd2dec(time_data[6]) + 100;
    return true;
}

void board_pcf85063_set_time(struct tm *now_tm)
{
    uint8_t time_data[7];

    if (!initialized || !now_tm)
    {
        return;
    }

    mktime(now_tm);
    time_data[0] = dec2bcd(now_tm->tm_sec) & 0x7F;
    time_data[1] = dec2bcd(now_tm->tm_min) & 0x7F;
    time_data[2] = dec2bcd(now_tm->tm_hour) & 0x3F;
    time_data[3] = dec2bcd(now_tm->tm_mday) & 0x3F;
    time_data[4] = dec2bcd(now_tm->tm_wday) & 0x07;
    time_data[5] = dec2bcd(now_tm->tm_mon + 1) & 0x1F;
    time_data[6] = dec2bcd((now_tm->tm_year - 100) % 100);
    pcf85063_reg_write(PCF85063_SECONDS, time_data, sizeof(time_data));
}
