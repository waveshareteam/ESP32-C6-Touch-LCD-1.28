#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "sd_card_bsp.h"
#include "driver/gpio.h"

#define PIN_NUM_MISO  (gpio_num_t)22
#define PIN_NUM_MOSI  (gpio_num_t)21
#define PIN_NUM_CLK   (gpio_num_t)11
#define SD_MOUNT_POINT "/sd_card"
#ifndef SDMMC_U
#define PIN_NUM_CS    (gpio_num_t)15
#define SD_SPI SPI2_HOST
#endif

static const char *TAG = "sd_card";
static sdmmc_card_t *s_sd_card = NULL;

#define SD_CARD_LOGI(format, ...) \
  do { \
    Serial.printf("[I][%s] " format "\n", TAG, ##__VA_ARGS__); \
  } while (0)

#define SD_CARD_LOGE(format, ...) \
  do { \
    Serial.printf("[E][%s] " format "\n", TAG, ##__VA_ARGS__); \
  } while (0)

esp_err_t sd_card_init(void)
{
  if (s_sd_card != NULL) {
    return ESP_OK;
  }

#ifdef SDMMC_U
  esp_vfs_fat_sdmmc_mount_config_t mount_config =
  {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 512,
    .disk_status_check_enable = 1,
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  slot_config.clk = PIN_NUM_CLK;
  slot_config.cmd = PIN_NUM_MOSI;
  slot_config.d0 = PIN_NUM_MISO;

  esp_err_t err = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_sd_card);
  if (err != ESP_OK) {
    SD_CARD_LOGE("Failed to mount SD card at %s: %s", SD_MOUNT_POINT, esp_err_to_name(err));
    return err;
  }

  sdmmc_card_print_info(stdout, s_sd_card);
  SD_CARD_LOGI("SD card capacity: %.2f GiB", sd_card_get_capacity_gb());
#endif

#ifndef SDMMC_U
  esp_vfs_fat_sdmmc_mount_config_t mount_config =
  {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 512,
  };
  spi_bus_config_t bus_cfg =
  {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
  };

  esp_err_t err = spi_bus_initialize(SD_SPI, &bus_cfg, SDSPI_DEFAULT_DMA);
  if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
    SD_CARD_LOGE("Failed to initialize SPI bus: %s", esp_err_to_name(err));
    return err;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_NUM_CS;
  slot_config.host_id = SD_SPI;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI;

  err = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_sd_card);
  if (err != ESP_OK) {
    SD_CARD_LOGE("Failed to mount SD card at %s: %s", SD_MOUNT_POINT, esp_err_to_name(err));
    return err;
  }

  sdmmc_card_print_info(stdout, s_sd_card);
  SD_CARD_LOGI("SD card capacity: %.2f GiB", sd_card_get_capacity_gb());
#endif

  return ESP_OK;
}

float sd_card_get_capacity_gb(void)
{
  if (s_sd_card != NULL) {
    return (float)(s_sd_card->csd.capacity) / 2048.0f / 1024.0f;
  }

  return 0.0f;
}

esp_err_t sd_card_write_file(const char *path, const char *data)
{
  if ((path == NULL) || (data == NULL)) {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_sd_card == NULL) {
    return ESP_ERR_NOT_FOUND;
  }

  esp_err_t err = sdmmc_get_status(s_sd_card);
  if (err != ESP_OK) {
    SD_CARD_LOGE("SD card status check failed before writing %s: %s", path, esp_err_to_name(err));
    return err;
  }

  FILE *file = fopen(path, "w");
  if (file == NULL) {
    SD_CARD_LOGE("Failed to open file for writing: %s", path);
    return ESP_ERR_NOT_FOUND;
  }

  size_t data_len = strlen(data);
  size_t written_len = fwrite(data, 1, data_len, file);
  if (written_len != data_len) {
    SD_CARD_LOGE("Failed to write all data to %s: wrote %u of %u bytes",
                 path, (unsigned int)written_len, (unsigned int)data_len);
    fclose(file);
    return ESP_FAIL;
  }

  if (fclose(file) != 0) {
    SD_CARD_LOGE("Failed to close file after writing: %s", path);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t sd_card_read_file(const char *path, uint8_t *buffer, uint32_t buffer_len, uint32_t *out_len)
{
  if ((path == NULL) || (buffer == NULL) || (out_len == NULL)) {
    return ESP_ERR_INVALID_ARG;
  }

  *out_len = 0;

  if (s_sd_card == NULL) {
    return ESP_ERR_NOT_FOUND;
  }

  esp_err_t err = sdmmc_get_status(s_sd_card);
  if (err != ESP_OK) {
    SD_CARD_LOGE("SD card status check failed before reading %s: %s", path, esp_err_to_name(err));
    return err;
  }

  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    SD_CARD_LOGE("Failed to open file for reading: %s", path);
    return ESP_ERR_NOT_FOUND;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    SD_CARD_LOGE("Failed to seek file end: %s", path);
    fclose(file);
    return ESP_FAIL;
  }

  long file_size = ftell(file);
  if (file_size < 0) {
    SD_CARD_LOGE("Failed to get file size: %s", path);
    fclose(file);
    return ESP_FAIL;
  }

  if (file_size > UINT32_MAX) {
    SD_CARD_LOGE("File too large for API: %s, size=%ld bytes", path, file_size);
    fclose(file);
    return ESP_ERR_INVALID_SIZE;
  }

  if ((uint32_t)file_size > buffer_len) {
    SD_CARD_LOGE("Buffer too small for %s: file=%ld bytes, buffer=%" PRIu32 " bytes",
                 path, file_size, buffer_len);
    fclose(file);
    return ESP_ERR_INVALID_SIZE;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    SD_CARD_LOGE("Failed to seek file start: %s", path);
    fclose(file);
    return ESP_FAIL;
  }

  size_t read_len = fread(buffer, 1, (size_t)file_size, file);
  if (read_len != (size_t)file_size) {
    SD_CARD_LOGE("Failed to read all data from %s: read %u of %ld bytes",
                 path, (unsigned int)read_len, file_size);
    fclose(file);
    return ESP_FAIL;
  }

  if (fclose(file) != 0) {
    SD_CARD_LOGE("Failed to close file after reading: %s", path);
    return ESP_FAIL;
  }

  *out_len = (uint32_t)read_len;
  SD_CARD_LOGI("Read %" PRIu32 " bytes from %s", *out_len, path);
  return ESP_OK;
}
