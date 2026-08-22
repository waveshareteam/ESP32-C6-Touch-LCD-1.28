#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "board_battery.h"
#include "board_pcf85063.h"
#include "board_qmi8658.h"
#include "board_sd.h"
#include "board_wifi.h"
#include "bsp/esp-bsp.h"

#include "iot_button.h"
#include "button_gpio.h"

#include "lvgl_ui.h"

static char *TAG = "factory";

static void button_init(void);
static void app_nvs_init(void);
static void app_peripherals_init(void);

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32-C6-Touch-LCD-1.28 factory UI");

    lv_display_t *disp = bsp_display_start();
    ESP_ERROR_CHECK(disp ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));

    app_nvs_init();
    app_peripherals_init();
    button_init();

    if (bsp_display_lock(0) == ESP_OK)
    {
        lvgl_ui_init();
        bsp_display_unlock();
    }
}

static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    ESP_LOGI(TAG, "BOOT KEY %s", iot_button_get_event_str(event));

    if (event == BUTTON_SINGLE_CLICK)
    {
        lvgl_ui_request_next_page();
    }
}

static void button_init(void)
{
    button_config_t btn_cfg = {};
    button_gpio_config_t btn_gpio_cfg = {};
    btn_gpio_cfg.gpio_num = BSP_BUTTON_BOOT;
    btn_gpio_cfg.active_level = 0;
    static button_handle_t btn = NULL;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn));
    iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
}

static void app_nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void app_peripherals_init(void)
{
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();

    ESP_ERROR_CHECK(board_battery_init());

    esp_err_t ret = board_qmi8658_init(i2c_bus);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "QMI8658 init failed: %s", esp_err_to_name(ret));
    }

    ret = board_pcf85063_init(i2c_bus);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "PCF85063 init failed: %s", esp_err_to_name(ret));
    }

    ret = board_sd_init();
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "SD card init failed: %s", esp_err_to_name(ret));
    }

    ret = board_wifi_init();
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
    }
}
