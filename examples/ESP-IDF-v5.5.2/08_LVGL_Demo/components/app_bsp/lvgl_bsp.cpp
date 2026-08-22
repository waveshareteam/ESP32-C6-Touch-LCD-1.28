#include "lvgl_bsp.h"

#include "esp_err.h"
#include "bsp/esp32_c6_touch_lcd_1_28.h"

esp_err_t lvgl_lock(int timeout_ms)
{
    return bsp_display_lock(timeout_ms);
}

void lvgl_unlock(void)
{
    bsp_display_unlock();
}

lv_display_t *lvgl_port_init(void)
{
    return bsp_display_start();
}

esp_err_t lvgl_refresh(void)
{
    lv_display_t *display = bsp_display_get_disp_dev();
    if (display == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    lv_refr_now(display);
    return ESP_OK;
}
