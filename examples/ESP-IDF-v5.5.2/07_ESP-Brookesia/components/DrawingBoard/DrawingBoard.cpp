/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "DrawingBoard.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:DrawingBoard"
#include "esp_lib_utils.h"

LV_IMG_DECLARE(image_draw);

#define APP_NAME "Drawing"

namespace esp_brookesia::apps
{
    DrawingBoard *DrawingBoard::_instance = nullptr;

    DrawingBoard *DrawingBoard::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        ESP_UTILS_LOGD("requestInstance called");
        if (_instance == nullptr)
        {
            _instance = new DrawingBoard(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    DrawingBoard::DrawingBoard(bool use_status_bar, bool use_navigation_bar)
        : App(APP_NAME, &image_draw, true, use_status_bar, use_navigation_bar)
    {
        ESP_UTILS_LOGD("Constructor called");
    }

    DrawingBoard::~DrawingBoard()
    {
        _resetCanvas();
        _instance = nullptr;
    }

    void DrawingBoard::_touch_event_cb(lv_event_t *e)
    {
        auto *self = static_cast<DrawingBoard *>(lv_event_get_user_data(e));
        if (self == nullptr) {
            return;
        }

        self->_handleTouchEvent(e);
    }

    void DrawingBoard::_draw_event_cb(lv_event_t *e)
    {
        auto *self = static_cast<DrawingBoard *>(lv_event_get_user_data(e));
        if (self == nullptr) {
            return;
        }

        self->_handleDrawEvent(e);
    }

    void DrawingBoard::_handleTouchEvent(lv_event_t *e)
    {
        const lv_event_code_t code = lv_event_get_code(e);
        if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) {
            return;
        }

        lv_indev_t *indev = lv_indev_get_act();
        if (indev == nullptr || _touch_panel == nullptr) {
            return;
        }

        lv_point_t point;
        lv_indev_get_point(indev, &point);
        ESP_UTILS_LOGD("Touch: x=%d, y=%d", point.x, point.y);

        if (code == LV_EVENT_PRESSED || !_has_last_point) {
            _has_last_point = true;
            _last_point = point;

            lv_point_t p2 = point;
            p2.x += (point.x < lv_obj_get_x2(_touch_panel)) ? 1 : -1;
            _appendStrokeSegment(point, p2);
            return;
        }

        if (_pointDistanceSquared(_last_point, point) < kMinPointDistance * kMinPointDistance) {
            return;
        }

        _appendStrokeSegment(_last_point, point);
        _last_point = point;
    }

    void DrawingBoard::_handleDrawEvent(lv_event_t *e)
    {
        lv_layer_t *layer = lv_event_get_layer(e);
        if (layer == nullptr || _segment_count == 0) {
            return;
        }

        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = lv_color_make(255, 0, 0);
        line_dsc.width = kPenWidth;
        line_dsc.opa = LV_OPA_COVER;
        line_dsc.round_start = true;
        line_dsc.round_end = true;

        const std::size_t start = (_segment_count == kMaxStrokeSegments) ? _segment_head : 0;
        for (std::size_t i = 0; i < _segment_count; ++i) {
            const StrokeSegment &segment = _segments[(start + i) % kMaxStrokeSegments];
            line_dsc.p1 = {segment.p1.x, segment.p1.y};
            line_dsc.p2 = {segment.p2.x, segment.p2.y};
            lv_draw_line(layer, &line_dsc);
        }
    }

    void DrawingBoard::_resetCanvas()
    {
        _segment_count = 0;
        _segment_head = 0;
        _has_last_point = false;

        if (_touch_panel != nullptr && lv_obj_is_valid(_touch_panel)) {
            lv_obj_delete(_touch_panel);
        }
        _touch_panel = nullptr;
    }

    void DrawingBoard::_appendStrokeSegment(lv_point_t p1, lv_point_t p2)
    {
        _segments[_segment_head] = {p1, p2};
        _segment_head = (_segment_head + 1) % kMaxStrokeSegments;
        if (_segment_count < kMaxStrokeSegments) {
            ++_segment_count;
        }

        if (_touch_panel != nullptr) {
            lv_obj_invalidate(_touch_panel);
        }
    }

    int32_t DrawingBoard::_pointDistanceSquared(const lv_point_t &p1, const lv_point_t &p2)
    {
        const int32_t dx = p1.x - p2.x;
        const int32_t dy = p1.y - p2.y;
        return dx * dx + dy * dy;
    }

    bool DrawingBoard::run(void)
    {
        ESP_UTILS_LOGD("Run function started");

        _resetCanvas();

        lv_area_t area = getVisualArea();
        int width = area.x2 - area.x1 + 1;
        int height = area.y2 - area.y1 + 1;

        _touch_panel = lv_obj_create(lv_scr_act());
        if (_touch_panel == nullptr) {
            ESP_UTILS_LOGE("Create drawing panel failed");
            return false;
        }

        lv_obj_set_size(_touch_panel, width, height);
        lv_obj_set_pos(_touch_panel, area.x1, area.y1);
        lv_obj_set_style_bg_opa(_touch_panel, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(_touch_panel, 0, 0);
        lv_obj_clear_flag(_touch_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(_touch_panel, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(_touch_panel, _touch_event_cb, LV_EVENT_PRESSED, this);
        lv_obj_add_event_cb(_touch_panel, _touch_event_cb, LV_EVENT_PRESSING, this);
        lv_obj_add_event_cb(_touch_panel, _draw_event_cb, LV_EVENT_DRAW_MAIN, this);

        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_VER);

        ESP_UTILS_LOGD("Run function finished successfully");
        return true;
    }

    bool DrawingBoard::back(void)
    {
        ESP_UTILS_LOGD("Back");
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool DrawingBoard::close(void)
    {
        ESP_UTILS_LOGD("Close");
        _resetCanvas();
        return true;
    }

    bool DrawingBoard::init()
    {
        ESP_UTILS_LOGD("Init");
        return true;
    }

    bool DrawingBoard::deinit()
    {
        ESP_UTILS_LOGD("Deinit");
        return true;
    }

    bool DrawingBoard::pause()
    {
        ESP_UTILS_LOGD("Pause");
        return true;
    }

    bool DrawingBoard::resume()
    {
        ESP_UTILS_LOGD("Resume");
        return true;
    }

} // namespace esp_brookesia::apps
