/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "systems/phone/widgets/recents_screen/esp_brookesia_recents_screen.hpp"
#include "systems/phone/assets/esp_brookesia_phone_assets.h"

namespace esp_brookesia::systems::phone {

constexpr RecentsScreenSnapshot::Data STYLESHEET_240_240_DARK_RECENTS_SCREEN_SNAPSHOT_DATA = {
    .main_size = gui::StyleSize::RECT(150, 150),
    .title = {
        .main_size = gui::StyleSize::RECT_W_PERCENT(100, 28),
        .main_layout_column_pad = 5,
        .icon_size = gui::StyleSize::SQUARE(20),
        .text_font = gui::StyleFont::SIZE(12),
        .text_color = gui::StyleColor::COLOR(0xFFFFFF),
    },
    .image = {
        .main_size = gui::StyleSize::RECT(150, 122),
        .radius = 16,
    },
    .flags = {
        .enable_all_main_size_refer_screen = 0,
    },
};

constexpr RecentsScreen::Data STYLESHEET_240_240_DARK_RECENTS_SCREEN_DATA = {
    .main = {
        .y_start = 0,
        .size = gui::StyleSize::RECT_PERCENT(100, 100),
        .layout_row_pad = 6,
        .layout_top_pad = 8,
        .layout_bottom_pad = 8,
        .background_color = gui::StyleColor::COLOR(0x1A1A1A),
    },
    .memory = {
        .main_size = gui::StyleSize::RECT_W_PERCENT(100, 16),
        .main_layout_x_right_offset = 6,
        .label_text_font = gui::StyleFont::HEIGHT_PERCENT(100),
        .label_text_color = gui::StyleColor::COLOR(0xFFFFFF),
        .label_unit_text = "KB",
    },
    .snapshot_table = {
        .main_size = gui::StyleSize::RECT_PERCENT(100, 100),
        .main_layout_column_pad = 16,
        .snapshot = STYLESHEET_240_240_DARK_RECENTS_SCREEN_SNAPSHOT_DATA,
    },
    .trash_icon = {
        .default_size = gui::StyleSize::SQUARE(34),
        .press_size = gui::StyleSize::SQUARE(30),
        .image = gui::StyleImage::IMAGE(&esp_brookesia_image_small_recents_screen_trash_38_38),
    },
    .flags = {
        .enable_memory = 1,
        .enable_table_height_flex = 1,
        .enable_table_snapshot_use_icon_image = 0,
        .enable_table_scroll_anim = 0,
    },
};

} // namespace esp_brookesia::systems::phone
