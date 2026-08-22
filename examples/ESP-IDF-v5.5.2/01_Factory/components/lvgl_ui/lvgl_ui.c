#include "lvgl_ui.h"
#include "tileview/qmi8658_tile.h"
#include "tileview/rgb_tile.h"
#include "tileview/system_tile.h"
#include "tileview/wifi_tile.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define UI_PAGE_COUNT 4
#define UI_WIFI_PAGE 3

static lv_obj_t *tileview;
static lv_obj_t *page_tiles[UI_PAGE_COUNT];
static uint8_t current_page;
static SemaphoreHandle_t next_page_sem;

static void tileview_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    lv_obj_t *active_tile = lv_tileview_get_tile_active(tileview);
    for (uint8_t i = 0; i < UI_PAGE_COUNT; i++)
    {
        if (active_tile == page_tiles[i])
        {
            current_page = i;
            break;
        }
    }

    if (current_page == UI_WIFI_PAGE)
    {
        wifi_tile_on_enter();
    }
}

static void next_page_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (next_page_sem && xSemaphoreTake(next_page_sem, 0) == pdTRUE)
    {
        lvgl_ui_next_page();
    }
}

void lvgl_ui_init(void)
{
    if (!next_page_sem)
    {
        next_page_sem = xSemaphoreCreateBinary();
    }

    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_style_bg_opa(tileview, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

    page_tiles[0] = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);
    rgb_tile_init(page_tiles[0]);

    page_tiles[1] = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    system_tile_init(page_tiles[1]);

    page_tiles[2] = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    qmi8658_tile_init(page_tiles[2]);

    page_tiles[3] = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_LEFT);
    wifi_tile_init(page_tiles[3]);

    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_timer_create(next_page_timer_cb, 20, NULL);
}

void lvgl_ui_next_page(void)
{
    if (!tileview)
    {
        return;
    }

    current_page = (current_page + 1) % UI_PAGE_COUNT;
    lv_obj_set_tile_id(tileview, current_page, 0, LV_ANIM_ON);

    if (current_page == UI_WIFI_PAGE)
    {
        wifi_tile_on_enter();
    }
}

void lvgl_ui_request_next_page(void)
{
    if (next_page_sem)
    {
        xSemaphoreGive(next_page_sem);
    }
}
