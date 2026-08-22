#include "set_wifi_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "wifi_ui";

static TaskHandle_t task_handle = NULL;
static lv_obj_t *list1 = NULL;
static lv_obj_t *spinner = NULL;
static volatile bool s_wifi_page_active = false;
static bool s_wifi_ready = false;
static volatile bool s_wifi_scan_cancelled = false;

static lv_obj_t *return_screen = NULL;
static lv_group_t *return_group = NULL;
static lv_group_t *wifi_group = NULL;
static lv_obj_t *wifi_con = NULL;
static lv_obj_t *back_btn = NULL;

#define WIFI_PANEL_W 196
#define WIFI_PANEL_H 196
#define WIFI_LIST_W 178
#define WIFI_LIST_H 130
#define WIFI_SCAN_MAX_AP 20

static esp_err_t wifi_scan_enable_sta(void)
{
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK) {
        return err;
    }

    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        return ESP_OK;
    }

    wifi_mode_t new_mode = (mode == WIFI_MODE_AP) ? WIFI_MODE_APSTA : WIFI_MODE_STA;
    err = esp_wifi_set_mode(new_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Set WiFi scan mode failed: %s", esp_err_to_name(err));
    }
    return err;
}

static esp_err_t wifi_scan_init(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == NULL) {
        esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
        return err;
    }

    err = wifi_scan_enable_sta();
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN && err != ESP_ERR_WIFI_STATE) {
        return err;
    }

    s_wifi_ready = true;
    return ESP_OK;
}

esp_err_t wifi_c5_init_and_connect(void)
{
    if (s_wifi_ready) {
        return ESP_OK;
    }

    esp_err_t err = wifi_scan_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(err));
    }
    return err;
}

static void scan_ssid(void)
{
    if (wifi_scan_init() != ESP_OK || !s_wifi_page_active) {
        return;
    }

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };

    esp_err_t stop_err = esp_wifi_scan_stop();
    if (stop_err != ESP_OK && stop_err != ESP_ERR_WIFI_NOT_INIT && stop_err != ESP_ERR_WIFI_STATE) {
        ESP_LOGD(TAG, "No previous WiFi scan to stop: %s", esp_err_to_name(stop_err));
    }

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK || !s_wifi_page_active || s_wifi_scan_cancelled) {
        ESP_LOGW(TAG, "WiFi scan skipped: %s", esp_err_to_name(err));
        return;
    }

    uint16_t ap_num = WIFI_SCAN_MAX_AP;
    wifi_ap_record_t ap_records[WIFI_SCAN_MAX_AP];
    memset(ap_records, 0, sizeof(ap_records));

    err = esp_wifi_scan_get_ap_records(&ap_num, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Get AP records failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Total APs scanned = %d", ap_num);

    if (bsp_display_lock(-1) != ESP_OK) {
        ESP_LOGW(TAG, "Skip WiFi list update, display lock failed");
        return;
    }

    if (!s_wifi_page_active || s_wifi_scan_cancelled || list1 == NULL) {
        bsp_display_unlock();
        return;
    }

    lv_obj_clean(list1);
    if (ap_num == 0) {
        lv_list_add_text(list1, "No WiFi found");
        bsp_display_unlock();
        return;
    }

    for (uint16_t i = 0; i < ap_num; i++) {
        if (!s_wifi_page_active || s_wifi_scan_cancelled || list1 == NULL) {
            break;
        }

        const char *band_str = (ap_records[i].primary > 14) ? "5G" : "2.4G";

        char label[64];
        snprintf(label, sizeof(label), "%s [%s] (%d dBm)",
                 (char *)ap_records[i].ssid,
                 band_str,
                 ap_records[i].rssi);

        lv_obj_t *btn = lv_list_add_button(list1, LV_SYMBOL_WIFI, label);
        if (btn == NULL) {
            ESP_LOGE(TAG, "Create WiFi list item failed");
            continue;
        }

        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, lv_color_hex(0x1F2933), LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xD1D5DB), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn, 6, LV_PART_MAIN);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, LV_PART_MAIN);

        if (wifi_group != NULL) {
            lv_group_add_obj(wifi_group, btn);
        }

        ESP_LOGD(TAG, "%s", label);
    }
    bsp_display_unlock();
}

static void wifi_handle_task(void *pvParameters)
{
    if (bsp_display_lock(-1) == ESP_OK) {
        if (s_wifi_page_active && spinner != NULL) {
            lv_obj_remove_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        }
        bsp_display_unlock();
    }

    scan_ssid();

    if (bsp_display_lock(-1) == ESP_OK) {
        if (s_wifi_page_active && spinner != NULL) {
            lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        }
        bsp_display_unlock();
    }

    task_handle = NULL;
    vTaskDelete(NULL);
}

static void wait_wifi_task_done(void)
{
    for (int i = 0; i < 50 && task_handle != NULL; i++) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (task_handle != NULL) {
        ESP_LOGW(TAG, "WiFi scan task did not stop before UI cleanup");
    }
}

static void back_btn_click_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        deinit_wifi_settings_screen();

        ESP_LOGI(TAG, "Restore settings input group");
        if (return_group) {
            lv_indev_set_group(bsp_display_get_input_dev(), return_group);
        }

        ESP_LOGI(TAG, "Load settings screen");
        lv_screen_load(return_screen);
    }
}

void init_wifi_settings_screen(lv_obj_t *parent, lv_obj_t *ret_scr, lv_group_t *ret_group)
{
    if (parent == NULL) {
        ESP_LOGE(TAG, "Invalid WiFi settings parent");
        return;
    }

    return_screen = ret_scr;
    return_group = ret_group;

    if (wifi_group != NULL) {
        lv_group_delete(wifi_group);
    }
    wifi_group = lv_group_create();
    if (wifi_group == NULL) {
        ESP_LOGE(TAG, "Create WiFi input group failed");
        return;
    }

    lv_obj_set_style_bg_color(parent, lv_color_hex(0xF7F8FA), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);

    wifi_con = lv_obj_create(parent);
    if (wifi_con == NULL) {
        ESP_LOGE(TAG, "Create WiFi container failed");
        return;
    }
    lv_obj_set_size(wifi_con, WIFI_PANEL_W, WIFI_PANEL_H);
    lv_obj_align(wifi_con, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(wifi_con, lv_color_hex(0xF7F8FA), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_con, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_con, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_con, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(wifi_con, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(wifi_con, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(wifi_con, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(wifi_con, 4, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_con, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(wifi_con, LV_OBJ_FLAG_SCROLLABLE);

    back_btn = lv_btn_create(wifi_con);
    if (back_btn == NULL) {
        ESP_LOGE(TAG, "Create WiFi back button failed");
        return;
    }
    lv_obj_set_size(back_btn, 34, 26);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 14, 6);
    lv_obj_add_event_cb(back_btn, back_btn_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(wifi_group, back_btn);

    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(back_btn, lv_color_hex(0xE5E7EB), LV_PART_MAIN);

    lv_obj_t *back_label = lv_label_create(back_btn);
    if (back_label == NULL) {
        ESP_LOGE(TAG, "Create WiFi back label failed");
        return;
    }
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0x1F2933), LV_PART_MAIN);
    lv_obj_center(back_label);

    lv_obj_t *title_label = lv_label_create(wifi_con);
    if (title_label == NULL) {
        ESP_LOGE(TAG, "Create WiFi title label failed");
        return;
    }
    lv_label_set_text(title_label, "WIFI SCAN");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x1F2933), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_opa(title_label, LV_OPA_100, 0);

    list1 = lv_list_create(wifi_con);
    if (list1 == NULL) {
        ESP_LOGE(TAG, "Create WiFi list failed");
        return;
    }
    lv_obj_set_size(list1, WIFI_LIST_W, WIFI_LIST_H);
    lv_obj_align(list1, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_bg_color(list1, lv_color_hex(0xF7F8FA), LV_PART_MAIN);
    lv_obj_set_style_text_color(list1, lv_color_hex(0x1F2933), LV_PART_MAIN);
    lv_obj_set_style_pad_top(list1, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(list1, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_left(list1, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_right(list1, 5, LV_PART_MAIN);
    lv_obj_set_style_border_color(list1, lv_color_hex(0xE5E7EB), LV_PART_MAIN);
    lv_obj_set_style_border_width(list1, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(list1, 4, LV_PART_MAIN);

    spinner = lv_spinner_create(wifi_con);
    if (spinner == NULL) {
        ESP_LOGE(TAG, "Create WiFi spinner failed");
        return;
    }
    lv_obj_set_size(spinner, 56, 56);
    lv_obj_center(spinner);
    lv_spinner_set_anim_params(spinner, 1000, 200);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
}

void open_wifi_page(void)
{
    s_wifi_page_active = true;
    s_wifi_scan_cancelled = false;
    if (wifi_group != NULL) {
        ESP_LOGI(TAG, "Set WiFi input group");
        lv_indev_set_group(bsp_display_get_input_dev(), wifi_group);
    }

    if (task_handle == NULL) {
        BaseType_t ok = xTaskCreate(wifi_handle_task, "wifi_scan_task", 4096, NULL, 3, &task_handle);
        if (ok != pdPASS) {
            task_handle = NULL;
            ESP_LOGE(TAG, "Create WiFi scan task failed");
        }
    }
}

void deinit_wifi_settings_screen(void)
{
    s_wifi_page_active = false;
    s_wifi_scan_cancelled = true;
    esp_err_t err = esp_wifi_scan_stop();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT && err != ESP_ERR_WIFI_STATE) {
        ESP_LOGW(TAG, "Stop WiFi scan failed: %s", esp_err_to_name(err));
    }
    wait_wifi_task_done();
    list1 = NULL;
    spinner = NULL;
    back_btn = NULL;
    wifi_con = NULL;
    if (wifi_group != NULL) {
        lv_group_delete(wifi_group);
        wifi_group = NULL;
    }
}
