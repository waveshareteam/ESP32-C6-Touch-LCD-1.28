idf_component_register(
    SRCS "display_no_touch.c" "main.cpp"
    REQUIRES esp32_c6_touch_lcd_1_28 espressif__esp_lvgl_adapter fatfs lvgl
    INCLUDE_DIRS ".")
