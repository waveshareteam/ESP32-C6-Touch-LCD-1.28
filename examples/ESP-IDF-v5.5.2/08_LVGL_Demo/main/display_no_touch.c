#include "display_no_touch.h"

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#include "bsp/esp32_c6_touch_lcd_1_28.h"

static const char *TAG = "display_no_touch";

lv_display_t *app_display_start_without_touch(void)
{
    esp_lv_adapter_config_t lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    esp_err_t ret = esp_lv_adapter_init(&lv_adapter_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL adapter init failed: %s", esp_err_to_name(ret));
        return NULL;
    }

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const bsp_display_config_t disp_config = {
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * BSP_LCD_BITS_PER_PIXEL / 8,
    };

    ret = bsp_display_new(&disp_config, &panel_handle, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display panel init failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    esp_lv_adapter_display_config_t disp_cfg =
        ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(panel_handle, io_handle,
                                                                BSP_LCD_H_RES, BSP_LCD_V_RES,
                                                                ESP_LV_ADAPTER_ROTATE_0);
    disp_cfg.profile.buffer_height = LVGL_BUFFER_HEIGHT;
    disp_cfg.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;

    lv_display_t *display = esp_lv_adapter_register_display(&disp_cfg);
    if (display == NULL) {
        ESP_LOGE(TAG, "LVGL display register failed");
        goto fail;
    }

    ret = bsp_display_brightness_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display brightness init failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL adapter start failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    ESP_LOGI(TAG, "Display started without touch input");
    return display;

fail:
    esp_lv_adapter_deinit();
    if (panel_handle != NULL) {
        esp_lcd_panel_del(panel_handle);
    }
    if (io_handle != NULL) {
        esp_lcd_panel_io_del(io_handle);
    }
    return NULL;
}
