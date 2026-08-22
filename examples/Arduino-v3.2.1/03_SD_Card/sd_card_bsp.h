#ifndef SD_CARD_BSP_H
#define SD_CARD_BSP_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize and mount the SD card.
 *
 * @return ESP_OK on success, or an ESP-IDF error code on failure.
 */
esp_err_t sd_card_init(void);

/**
 * @brief Get the mounted SD card capacity in GiB.
 *
 * @return Capacity in GiB, or 0.0f if the card is not mounted.
 */
float sd_card_get_capacity_gb(void);

/**
 * @brief Write a null-terminated string to a file on the SD card.
 *
 * @param path Absolute file path under the SD mount point.
 * @param data Null-terminated string to write.
 * @return ESP_OK on success, or an ESP-IDF error code on failure.
 */
esp_err_t sd_card_write_file(const char *path, const char *data);

/**
 * @brief Read a file from the SD card into a caller-provided buffer.
 *
 * @param path Absolute file path under the SD mount point.
 * @param buffer Destination buffer.
 * @param buffer_len Destination buffer capacity in bytes.
 * @param out_len Number of bytes read.
 * @return ESP_OK on success, or an ESP-IDF error code on failure.
 */
esp_err_t sd_card_read_file(const char *path, uint8_t *buffer, uint32_t buffer_len, uint32_t *out_len);

#endif
