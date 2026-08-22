#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "sd_test";

#define SD_MOUNT_POINT      "/sdcard"
#define SD_SPI_HOST         SPI2_HOST

/* uSD card pinout for ESP32-C6-Touch-LCD-1.28. */
#define SD_PIN_MOSI         GPIO_NUM_21
#define SD_PIN_SCLK         GPIO_NUM_11
#define SD_PIN_MISO         GPIO_NUM_22
#define SD_PIN_CS           GPIO_NUM_15
#define SD_MAX_TRANSFER_SZ  (240 * 240 * sizeof(uint16_t))

static sdmmc_card_t *sd_card;

static esp_err_t sd_card_mount(void)
{
    const spi_bus_config_t bus_config = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_SCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = SD_MAX_TRANSFER_SZ,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO),
                        TAG, "SPI bus init failed");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    const sdspi_device_config_t slot_config = {
        .host_id = SD_SPI_HOST,
        .gpio_cs = SD_PIN_CS,
        .gpio_cd = SDSPI_SLOT_NO_CD,
        .gpio_wp = SDSPI_SLOT_NO_WP,
        .gpio_int = GPIO_NUM_NC,
    };

    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config,
                                            &mount_config, &sd_card);
    if (ret != ESP_OK) {
        spi_bus_free(SD_SPI_HOST);
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted at %s, name=%s", SD_MOUNT_POINT, sd_card->cid.name);
    return ESP_OK;
}

static esp_err_t sd_card_rw_test(void)
{
    const char *file_path = SD_MOUNT_POINT "/test.txt";
    const char *write_data = "Hello ESP32! TF card write test.\n";
    const size_t write_len = strlen(write_data);

    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s for writing: errno=%d", file_path, errno);
        return ESP_FAIL;
    }

    size_t written = fwrite(write_data, 1, write_len, f);
    if (written != write_len) {
        ESP_LOGE(TAG, "Failed to write %s: written=%u expected=%u errno=%d",
                 file_path, (unsigned int)written, (unsigned int)write_len, errno);
        fclose(f);
        return ESP_FAIL;
    }

    if (fclose(f) != 0) {
        ESP_LOGE(TAG, "Failed to close %s after writing: errno=%d", file_path, errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Write done: %s", file_path);

    f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s for reading: errno=%d", file_path, errno);
        return ESP_FAIL;
    }

    char read_buf[128];
    memset(read_buf, 0, sizeof(read_buf));
    size_t read_len = fread(read_buf, 1, sizeof(read_buf) - 1, f);
    if (ferror(f)) {
        ESP_LOGE(TAG, "Failed to read %s: errno=%d", file_path, errno);
        fclose(f);
        return ESP_FAIL;
    }

    if (fclose(f) != 0) {
        ESP_LOGE(TAG, "Failed to close %s after reading: errno=%d", file_path, errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Read done: %s", read_buf);

    if (read_len == write_len && memcmp(read_buf, write_data, write_len) == 0) {
        ESP_LOGI(TAG, "Read/Write test PASSED!");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Read/Write test FAILED: read=%u expected=%u",
                 (unsigned int)read_len, (unsigned int)write_len);
        return ESP_FAIL;
    }
}

void app_main(void)
{
    esp_err_t ret = sd_card_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = sd_card_rw_test();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card read/write test failed: %s", esp_err_to_name(ret));
        return;
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
