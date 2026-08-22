#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "lvgl.h"

#include <array>
#include <cstddef>

namespace esp_brookesia::apps
{
    class DrawingBoard : public systems::phone::App
    {
    public:
        static DrawingBoard *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~DrawingBoard();

    protected:
        DrawingBoard(bool use_status_bar, bool use_navigation_bar);

        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        struct StrokeSegment {
            lv_point_t p1;
            lv_point_t p2;
        };

        static constexpr std::size_t kMaxStrokeSegments = 1024;
        static constexpr int32_t kPenWidth = 10;
        static constexpr int32_t kMinPointDistance = 2;

        static DrawingBoard *_instance;
        static void _touch_event_cb(lv_event_t *e);
        static void _draw_event_cb(lv_event_t *e);

        void _handleTouchEvent(lv_event_t *e);
        void _handleDrawEvent(lv_event_t *e);
        void _resetCanvas();
        void _appendStrokeSegment(lv_point_t p1, lv_point_t p2);
        static int32_t _pointDistanceSquared(const lv_point_t &p1, const lv_point_t &p2);

        lv_obj_t *_touch_panel = nullptr;
        std::array<StrokeSegment, kMaxStrokeSegments> _segments = {};
        std::size_t _segment_count = 0;
        std::size_t _segment_head = 0;
        lv_point_t _last_point = {};
        bool _has_last_point = false;
    };
}
