#include "system_tile.h"

#include "board_battery.h"
#include "board_pcf85063.h"
#include "board_sd.h"
#include "bsp/esp-bsp.h"

#include "esp_flash.h"
#include "esp_timer.h"
#include "driver/temperature_sensor.h"
#include "esp_private/esp_clk.h"

static lv_obj_t *label_battery;
static lv_obj_t *label_chip_temp;
static lv_obj_t *label_rtc_time;
static lv_obj_t *label_sd_card;
static lv_obj_t *label_uptime;
static lv_obj_t *brightness_slider;

static temperature_sensor_handle_t temp_sensor = NULL;

static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_obj_t *slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);

        bsp_display_brightness_set(value);
        lv_event_stop_bubbling(e);
    }
}

static void system_time_cb(lv_timer_t *timer)
{
    char str[20];
    float tsens_out;
    float battery_voltage = 0.0f;
    uint16_t battery_adc = 0;
    uint64_t sd_total_bytes = 0;
    struct tm now_tm;

    if (temp_sensor && temperature_sensor_get_celsius(temp_sensor, &tsens_out) == ESP_OK)
    {
        sprintf(str, "%d C", (int)tsens_out);
        lv_label_set_text(label_chip_temp, str);
    }

    if (board_battery_read(&battery_adc, &battery_voltage) == ESP_OK)
    {
        int battery_mv = (int)(battery_voltage * 1000.0f + 0.5f);
        lv_label_set_text_fmt(label_battery, "%d.%02d V", battery_mv / 1000, (battery_mv % 1000) / 10);
    }

    if (board_sd_get_capacity(&sd_total_bytes) == ESP_OK)
    {
        uint32_t sd_total_mb = (uint32_t)(sd_total_bytes / (1024ULL * 1024ULL));
        if (sd_total_mb >= 1024)
        {
            lv_label_set_text_fmt(label_sd_card, "%lu.%lu GB",
                                  (unsigned long)(sd_total_mb / 1024),
                                  (unsigned long)((sd_total_mb % 1024) * 10 / 1024));
        }
        else
        {
            lv_label_set_text_fmt(label_sd_card, "%lu MB", (unsigned long)sd_total_mb);
        }
    }

    if (board_pcf85063_get_time(&now_tm))
    {
        lv_label_set_text_fmt(label_rtc_time, "%02d:%02d:%02d", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
    }

    int64_t uptime_s = esp_timer_get_time() / 1000000;
    lv_label_set_text_fmt(label_uptime, "%lld s", (long long)uptime_s);
}

void system_tile_init(lv_obj_t *parent)
{
    uint32_t flash_size;
    uint32_t cpu_freq;
    int brightness;

    brightness = bsp_display_brightness_get();
    if (brightness < 0)
    {
        brightness = 100;
    }
    lv_obj_t *list = lv_list_create(parent);
    lv_obj_t *lable = lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "System");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lv_obj_set_size(list, lv_pct(100), lv_pct(70));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 30);

    brightness_slider = lv_slider_create(parent);
    lv_slider_set_range(brightness_slider, 1, 100);
    lv_slider_set_value(brightness_slider, brightness, LV_ANIM_OFF);

    lv_obj_set_size(brightness_slider, lv_pct(70), lv_pct(7));
    lv_obj_align(brightness_slider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(brightness_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *list_item;
    list_item = lv_list_add_btn(list, NULL, "Chip");
    lv_obj_t *label_chip = lv_label_create(list_item);
    lv_label_set_text(label_chip, "ESP32-C6");

    list_item = lv_list_add_btn(list, NULL, "Temp");
    label_chip_temp = lv_label_create(list_item);
    lv_label_set_text(label_chip_temp, "--- C");

    list_item = lv_list_add_btn(list, NULL, "SD Card");
    label_sd_card = lv_label_create(list_item);
    lv_label_set_text(label_sd_card, "No card");

    list_item = lv_list_add_btn(list, NULL, "Battary ADC");
    label_battery = lv_label_create(list_item);
    lv_label_set_text(label_battery, "-- V");

    list_item = lv_list_add_btn(list, NULL, "RTC Time");
    label_rtc_time = lv_label_create(list_item);
    lv_label_set_text(label_rtc_time, "--:--:--");

    list_item = lv_list_add_btn(list, NULL, "Freq");
    lv_obj_t *label_chip_freq = lv_label_create(list_item);
    cpu_freq = esp_clk_cpu_freq();
    lv_label_set_text_fmt(label_chip_freq, "%d MHz", (int)(cpu_freq / 1000 / 1000));

    list_item = lv_list_add_btn(list, NULL, "RAM");
    lv_obj_t *label_ram = lv_label_create(list_item);
    lv_label_set_text(label_ram, "512 KB");

    list_item = lv_list_add_btn(list, NULL, "Flash");
    lv_obj_t *label_flash = lv_label_create(list_item);
    esp_flash_get_size(NULL, &flash_size);
    lv_label_set_text_fmt(label_flash, "%d MB", (int)(flash_size / 1024 / 1024));

    list_item = lv_list_add_btn(list, NULL, "LCD IC");
    lv_obj_t *label_lcd_ic = lv_label_create(list_item);
    lv_label_set_text(label_lcd_ic, "GC9A01");

    list_item = lv_list_add_btn(list, NULL, "Touch");
    lv_obj_t *label_touch = lv_label_create(list_item);
    lv_label_set_text(label_touch, bsp_display_get_input_dev() ? "CST816S" : "Not ready");

    list_item = lv_list_add_btn(list, NULL, "Uptime");
    label_uptime = lv_label_create(list_item);
    lv_label_set_text(label_uptime, "--- s");

    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
    if (temperature_sensor_install(&temp_sensor_config, &temp_sensor) == ESP_OK)
    {
        temperature_sensor_enable(temp_sensor);
    }

    lv_timer_create(system_time_cb, 1000, NULL);
}
