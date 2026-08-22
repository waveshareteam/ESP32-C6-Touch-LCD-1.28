#include "wifi_tile.h"

#include <string.h>

#include "board_wifi.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LIST_BTN_LEN_MAX 12

static lv_obj_t *list;
static lv_obj_t *label_wifi_ip;
static lv_obj_t *list_btns[LIST_BTN_LEN_MAX];
static uint16_t list_item_count;
static SemaphoreHandle_t wifi_scan_sem;
static SemaphoreHandle_t wifi_connect_sem;

static void clear_scan_list(void)
{
    for (int i = 0; i < list_item_count && i < LIST_BTN_LEN_MAX; i++)
    {
        if (list_btns[i])
        {
            lv_obj_del(list_btns[i]);
            list_btns[i] = NULL;
        }
    }
    list_item_count = 0;
}

static void add_status_item(const char *text)
{
    clear_scan_list();
    list_btns[list_item_count++] = lv_list_add_btn(list, NULL, text);
}

static void btn_wifi_scan_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        add_status_item("Scanning...");
        if (wifi_scan_sem)
        {
            xSemaphoreGive(wifi_scan_sem);
        }
    }
}

static void wifi_time_cb(lv_timer_t *timer)
{
    char ip[16] = {0};
    board_wifi_get_ip(ip);
    lv_label_set_text_fmt(label_wifi_ip, "IP: %s", ip);
}

static void lvgl_wifi_task(void *arg)
{
    wifi_ap_record_t ap_info[LIST_BTN_LEN_MAX];
    uint16_t ap_count = 0;

    while (1)
    {
        if (xSemaphoreTake(wifi_scan_sem, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        memset(ap_info, 0, sizeof(ap_info));
        bool ok = board_wifi_scan(ap_info, &ap_count, LIST_BTN_LEN_MAX);

        if (bsp_display_lock(1000) != ESP_OK)
        {
            continue;
        }

        clear_scan_list();
        if (!ok || ap_count == 0)
        {
            add_status_item(ok ? "No AP found" : "Scan failed");
        }
        else
        {
            for (int i = 0; i < ap_count && i < LIST_BTN_LEN_MAX; i++)
            {
                list_btns[i] = lv_list_add_btn(list, NULL, (const char *)ap_info[i].ssid);
                lv_obj_t *label = lv_label_create(list_btns[i]);
                lv_label_set_text_fmt(label, "%d dBm", ap_info[i].rssi);
                list_item_count++;
            }
        }
        bsp_display_unlock();
    }
}

static void wifi_connect_task(void *arg)
{
    while (1)
    {
        xSemaphoreTake(wifi_connect_sem, portMAX_DELAY);
        board_wifi_connect_default();
    }
}

void wifi_tile_on_enter(void)
{
    if (wifi_connect_sem)
    {
        xSemaphoreGive(wifi_connect_sem);
    }
}

void wifi_tile_init(lv_obj_t *parent)
{
    list = lv_list_create(parent);

    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(label, "WiFi");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 3);

    label_wifi_ip = lv_label_create(parent);
    lv_label_set_text(label_wifi_ip, "IP: 0.0.0.0");
    lv_obj_align(label_wifi_ip, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_set_size(list, lv_pct(100), lv_pct(60));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, lv_pct(80), 40);
    label = lv_label_create(btn);
    lv_label_set_text(label, "scan");
    lv_obj_center(label);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, btn_wifi_scan_event_handler, LV_EVENT_CLICKED, NULL);

    wifi_scan_sem = xSemaphoreCreateBinary();
    wifi_connect_sem = xSemaphoreCreateBinary();
    if (wifi_scan_sem)
    {
        xTaskCreate(lvgl_wifi_task, "lvgl_wifi_task", 4096, NULL, 2, NULL);
    }
    if (wifi_connect_sem)
    {
        xTaskCreate(wifi_connect_task, "wifi_connect_task", 4096, NULL, 2, NULL);
    }
    lv_timer_create(wifi_time_cb, 1000, NULL);
}
