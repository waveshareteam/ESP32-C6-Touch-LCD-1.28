/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "qmi8658.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace esp_brookesia::apps
{
    class Qmi8658Ball : public systems::phone::App
    {
    public:
        static Qmi8658Ball *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~Qmi8658Ball();

        using systems::phone::App::endRecordResource;
        using systems::phone::App::startRecordResource;

    protected:
        Qmi8658Ball(bool use_status_bar, bool use_navigation_bar);
        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static Qmi8658Ball *_instance;

        lv_obj_t *_ball = nullptr;
        lv_obj_t *_calib_label = nullptr;
        lv_obj_t *_calib_progress_label = nullptr;
        lv_obj_t *_calib_progress_bar = nullptr;
        lv_area_t _screen_area = {0};

        qmi8658_dev_t *_qmi8658_dev;
        TaskHandle_t _update_task_handle;
        TaskHandle_t _calibration_task_handle;
        i2c_master_bus_handle_t _i2c_bus;
        bool _calibration_done;
        volatile bool _recalibration_requested;
        volatile bool _closing;
        bool _calib_button_isr_registered;

        float _accel_bias_x;
        float _accel_bias_y;
        int _display_width;
        int _display_height;

        enum CalibState
        {
            CALIB_IDLE,
            CALIB_RUNNING,
            CALIB_DONE,
            CALIB_FAILED
        } _calib_state;

        int _calib_progress;
        lv_timer_t *_calibration_ui_update_timer;

        static constexpr int kBallRadius = 12;
        static constexpr int kAccelScaleFactor = 6;
        static constexpr int kTaskDelayMs = 30;
        static constexpr float kCalibrationDeadzone = 0.05f;
        static constexpr gpio_num_t kCalibButtonGpio = GPIO_NUM_9;

        void update_calibration_ui(void);
        void stop_tasks(void);

        static void ball_update_task(void *arg);
        static void calibration_task(void *arg);
        static void IRAM_ATTR button_isr_handler(void *arg);
        static void calibration_ui_update_timer_cb(lv_timer_t *timer);

        bool init_sensor(void);
        bool init_calibration_button(void);
        void perform_level_calibration(void);
        void apply_calibration_and_deadzone(qmi8658_data_t *data);
        void constrain_to_rounded_rect(int *x, int *y);
        bool create_ui_elements(void);
    };
} // namespace esp_brookesia::apps
