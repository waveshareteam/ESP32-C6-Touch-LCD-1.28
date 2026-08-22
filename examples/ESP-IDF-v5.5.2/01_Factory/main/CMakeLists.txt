idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp32_c6_touch_lcd_1_28"
                             "board_peripherals"
                             "lvgl_ui"
                             "espressif__button"
                             "nvs_flash")
