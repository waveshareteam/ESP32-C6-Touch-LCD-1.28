idf_component_register(
    SRCS "lvgl_bsp.cpp"
    REQUIRES esp32_c6_touch_lcd_1_28 fatfs lvgl
    INCLUDE_DIRS ".")
