#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the LVGL display without touch input support.
 *
 * @return Pointer to the LVGL display on success, or NULL on failure.
 */
lv_display_t *app_display_start_without_touch(void);

#ifdef __cplusplus
}
#endif
