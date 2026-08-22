#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the board display and LVGL port.
 *
 * @return Pointer to the LVGL display on success, or NULL on failure.
 */
lv_display_t *lvgl_port_init(void);

/**
 * @brief Refresh the active board display immediately.
 *
 * @return ESP_OK on success, or ESP_ERR_INVALID_STATE when the display is not initialized.
 */
esp_err_t lvgl_refresh(void);

/**
 * @brief Lock LVGL display access.
 *
 * @param timeout_ms Lock timeout in milliseconds.
 * @return ESP_OK on success, or an ESP-IDF error code on failure.
 */
esp_err_t lvgl_lock(int timeout_ms);

/**
 * @brief Unlock LVGL display access.
 */
void lvgl_unlock(void);

#ifdef __cplusplus
}
#endif
