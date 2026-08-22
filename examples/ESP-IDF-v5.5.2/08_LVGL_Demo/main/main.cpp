
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

#include "display_no_touch.h"
#include "lv_demos.h"
#include "bsp/esp32_c6_touch_lcd_1_28.h"

static const char *TAG = "main";

#define BRIGHTNESS_TEST_ENABLE  0
// 0: disable touch, works on both touch and non-touch screens.
// 1: enable touch, requires a CST816S touch controller.
#define APP_TOUCH_ENABLE 1

#if (BRIGHTNESS_TEST_ENABLE == 1)

static void brightness_test_task(void *arg)
{
    (void)arg;
    uint8_t back = 100;
    while (true) {
        if (bsp_display_lock(0) == ESP_OK) {
            esp_err_t ret = bsp_display_brightness_set(back);
            bsp_display_unlock();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Set display brightness to %u failed: %s",
                         back, esp_err_to_name(ret));
            }
        } else {
            ESP_LOGW(TAG, "Display lock failed during brightness test");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        if (back > 0) {
            back = back - 20;
        } else {
            back = 100;
        }
    }
}

#endif

extern "C" void app_main(void)
{
#if (APP_TOUCH_ENABLE == 1)
    lv_display_t *display = bsp_display_start();
#else
    lv_display_t *display = app_display_start_without_touch();
#endif
    if (display == NULL) {
        ESP_LOGE(TAG, "Display init failed");
        return;
    }

    esp_err_t ret = bsp_display_lock(0);
    if (ret == ESP_OK) {
        lv_demo_widgets();
        bsp_display_unlock();
    } else {
        ESP_LOGE(TAG, "Display lock failed before loading demo widgets: %s", esp_err_to_name(ret));
    }
#if (BRIGHTNESS_TEST_ENABLE == 1)
    BaseType_t task_ret = xTaskCreatePinnedToCore(brightness_test_task, "brightness_test",
                                                  3 * 1024, NULL, 3, NULL, 0);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Create brightness test task failed: %d", task_ret);
    }
#endif
}
