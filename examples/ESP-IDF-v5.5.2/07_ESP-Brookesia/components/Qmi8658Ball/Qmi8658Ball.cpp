/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Qmi8658Ball.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "esp_lib_utils.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "driver/gpio.h"
#include <cstring>
#include <cmath>

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Qmi8658Ball"

LV_IMG_DECLARE(img_app_qmi8658ball);

namespace esp_brookesia::apps
{

    Qmi8658Ball *Qmi8658Ball::_instance = nullptr;

    Qmi8658Ball *Qmi8658Ball::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new Qmi8658Ball(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    Qmi8658Ball::Qmi8658Ball(bool use_status_bar, bool use_navigation_bar) : App("GravityBall", &img_app_qmi8658ball, true, use_status_bar, use_navigation_bar),
                                                                             _ball(nullptr),
                                                                             _calib_label(nullptr),
                                                                             _calib_progress_label(nullptr),
                                                                             _calib_progress_bar(nullptr),
                                                                             _screen_area({0}),
                                                                             _qmi8658_dev(nullptr),
                                                                             _update_task_handle(nullptr),
                                                                             _calibration_task_handle(nullptr),
                                                                             _i2c_bus(nullptr),
                                                                             _calibration_done(false),
                                                                             _recalibration_requested(false),
                                                                             _closing(false),
                                                                             _calib_button_isr_registered(false),
                                                                             _accel_bias_x(0.0f),
                                                                             _accel_bias_y(0.0f),
                                                                             _display_width(0),
                                                                             _display_height(0),
                                                                             _calib_state(CALIB_IDLE),
                                                                             _calib_progress(0),
                                                                             _calibration_ui_update_timer(nullptr)
    {
    }

    Qmi8658Ball::~Qmi8658Ball()
    {
        stop_tasks();
        if (_qmi8658_dev)
        {
            if (_qmi8658_dev->dev_handle != nullptr)
            {
                i2c_master_bus_rm_device(_qmi8658_dev->dev_handle);
            }
            free(_qmi8658_dev);
            _qmi8658_dev = nullptr;
        }
        if (_calib_button_isr_registered)
        {
            gpio_isr_handler_remove(kCalibButtonGpio);
            _calib_button_isr_registered = false;
        }
    }

    void Qmi8658Ball::stop_tasks(void)
    {
        _closing = true;
        _recalibration_requested = false;

        if (_update_task_handle)
        {
            vTaskResume(_update_task_handle);
            xTaskNotifyGive(_update_task_handle);
        }

        const TickType_t wait_step = pdMS_TO_TICKS(20);
        for (int i = 0; i < 50 && (_update_task_handle || _calibration_task_handle); i++)
        {
            vTaskDelay(wait_step);
        }

        if (_update_task_handle)
        {
            ESP_UTILS_LOGW("Force delete ball update task after timeout");
            vTaskDelete(_update_task_handle);
            _update_task_handle = nullptr;
        }
        if (_calibration_task_handle)
        {
            ESP_UTILS_LOGW("Force delete calibration task after timeout");
            vTaskDelete(_calibration_task_handle);
            _calibration_task_handle = nullptr;
        }
    }

    bool Qmi8658Ball::init(void)
    {
        ESP_UTILS_LOGD("Init");
        return true;
    }

    bool Qmi8658Ball::init_sensor(void)
    {
        if (_qmi8658_dev != nullptr)
        {
            return true;
        }

        _i2c_bus = bsp_i2c_get_handle();
        ESP_UTILS_CHECK_NULL_RETURN(_i2c_bus, false, "Get I2C bus failed");

        _qmi8658_dev = (qmi8658_dev_t *)malloc(sizeof(qmi8658_dev_t));
        ESP_UTILS_CHECK_NULL_RETURN(_qmi8658_dev, false, "Allocate QMI8658 device failed");
        std::memset(_qmi8658_dev, 0, sizeof(qmi8658_dev_t));

        esp_err_t ret = qmi8658_init(_qmi8658_dev, _i2c_bus, QMI8658_ADDRESS_HIGH);
        if (ret != ESP_OK)
        {
            ESP_UTILS_LOGE("Init QMI8658 failed: %s", esp_err_to_name(ret));
            if (_qmi8658_dev->dev_handle != nullptr)
            {
                i2c_master_bus_rm_device(_qmi8658_dev->dev_handle);
            }
            free(_qmi8658_dev);
            _qmi8658_dev = nullptr;
            return false;
        }

        ret = qmi8658_set_accel_range(_qmi8658_dev, QMI8658_ACCEL_RANGE_8G);
        if (ret != ESP_OK)
        {
            ESP_UTILS_LOGE("Set QMI8658 accel range failed: %s", esp_err_to_name(ret));
            i2c_master_bus_rm_device(_qmi8658_dev->dev_handle);
            free(_qmi8658_dev);
            _qmi8658_dev = nullptr;
            return false;
        }
        ret = qmi8658_set_accel_odr(_qmi8658_dev, QMI8658_ACCEL_ODR_500HZ);
        if (ret != ESP_OK)
        {
            ESP_UTILS_LOGE("Set QMI8658 accel ODR failed: %s", esp_err_to_name(ret));
            i2c_master_bus_rm_device(_qmi8658_dev->dev_handle);
            free(_qmi8658_dev);
            _qmi8658_dev = nullptr;
            return false;
        }
        qmi8658_set_accel_unit_mps2(_qmi8658_dev, true);
        ret = qmi8658_write_register(_qmi8658_dev, QMI8658_CTRL5, 0x03);
        if (ret != ESP_OK)
        {
            ESP_UTILS_LOGE("Set QMI8658 low-pass filter failed: %s", esp_err_to_name(ret));
            i2c_master_bus_rm_device(_qmi8658_dev->dev_handle);
            free(_qmi8658_dev);
            _qmi8658_dev = nullptr;
            return false;
        }

        ESP_UTILS_CHECK_FALSE_RETURN(init_calibration_button(), false, "Init calibration button failed");
        return true;
    }

    bool Qmi8658Ball::run(void)
    {
        ESP_UTILS_LOGD("Run");
        _closing = false;
        _recalibration_requested = false;
        ESP_UTILS_CHECK_FALSE_RETURN(init_sensor(), false, "Init sensor failed");

        _screen_area = getVisualArea();
        _display_width = _screen_area.x2 - _screen_area.x1;
        _display_height = _screen_area.y2 - _screen_area.y1;

        ESP_UTILS_CHECK_FALSE_RETURN(create_ui_elements(), false, "Create UI elements failed");

        _calib_state = CALIB_RUNNING;
        _calib_progress = 0;

        BaseType_t task_created = xTaskCreatePinnedToCore(
            calibration_task,
            "calibration_task",
            4096,
            this,
            5,
            &_calibration_task_handle,
            0);
        ESP_UTILS_CHECK_FALSE_RETURN(task_created == pdPASS, false, "Create calibration task failed");

        return true;
    }

    bool Qmi8658Ball::back(void)
    {
        ESP_UTILS_LOGD("Back");
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool Qmi8658Ball::close(void)
    {
        ESP_UTILS_LOGD("Close");
        stop_tasks();
        if (_ball)
        {
            lv_obj_del(_ball);
            _ball = nullptr;
        }
        if (_calib_label)
        {
            lv_obj_del(_calib_label);
            _calib_label = nullptr;
        }
        if (_calib_progress_label)
        {
            lv_obj_del(_calib_progress_label);
            _calib_progress_label = nullptr;
        }
        if (_calib_progress_bar)
        {
            lv_obj_del(_calib_progress_bar);
            _calib_progress_bar = nullptr;
        }
        if (_calibration_ui_update_timer)
        {
            lv_timer_del(_calibration_ui_update_timer);
            _calibration_ui_update_timer = nullptr;
        }
        return true;
    }

    bool Qmi8658Ball::deinit(void)
    {
        ESP_UTILS_LOGD("Deinit");
        return true;
    }

    bool Qmi8658Ball::pause(void)
    {
        ESP_UTILS_LOGD("Pause");
        if (_update_task_handle)
        {
            vTaskSuspend(_update_task_handle);
        }
        return true;
    }

    bool Qmi8658Ball::resume(void)
    {
        ESP_UTILS_LOGD("Resume");
        if (_update_task_handle)
        {
            vTaskResume(_update_task_handle);
        }
        return true;
    }

    bool Qmi8658Ball::create_ui_elements(void)
    {
        esp_err_t lock_ret = bsp_display_lock(pdMS_TO_TICKS(200));
        ESP_UTILS_CHECK_FALSE_RETURN(lock_ret == ESP_OK, false, "Lock display failed");

        auto cleanup_created = [this]() {
            if (_ball != nullptr) {
                lv_obj_del(_ball);
                _ball = nullptr;
            }
            if (_calib_label != nullptr) {
                lv_obj_del(_calib_label);
                _calib_label = nullptr;
            }
            if (_calib_progress_label != nullptr) {
                lv_obj_del(_calib_progress_label);
                _calib_progress_label = nullptr;
            }
            if (_calib_progress_bar != nullptr) {
                lv_obj_del(_calib_progress_bar);
                _calib_progress_bar = nullptr;
            }
        };

        _calib_progress_bar = lv_bar_create(lv_scr_act());
        if (_calib_progress_bar == nullptr)
        {
            cleanup_created();
            bsp_display_unlock();
            ESP_UTILS_LOGE("Create calibration progress bar failed");
            return false;
        }
        lv_obj_set_size(_calib_progress_bar, 150, 16);
        lv_obj_align(_calib_progress_bar, LV_ALIGN_CENTER, 0, -12);
        lv_bar_set_range(_calib_progress_bar, 0, 100);

        _calib_progress_label = lv_label_create(lv_scr_act());
        if (_calib_progress_label == nullptr)
        {
            cleanup_created();
            bsp_display_unlock();
            ESP_UTILS_LOGE("Create calibration progress label failed");
            return false;
        }
        lv_label_set_text(_calib_progress_label, "Calibrate 0%");
        lv_obj_set_style_text_font(_calib_progress_label, &lv_font_montserrat_12, 0);
        lv_obj_align_to(_calib_progress_label, _calib_progress_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

        _calib_label = lv_label_create(lv_scr_act());
        if (_calib_label == nullptr)
        {
            cleanup_created();
            bsp_display_unlock();
            ESP_UTILS_LOGE("Create calibration label failed");
            return false;
        }
        lv_label_set_text(_calib_label, "BOOT: recalibrate");
        lv_obj_set_style_text_font(_calib_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_calib_label, lv_color_hex(0xF2F2F2), 0);
        lv_obj_align(_calib_label, LV_ALIGN_TOP_MID, 0, 28);
        lv_obj_add_flag(_calib_label, LV_OBJ_FLAG_HIDDEN);

        _ball = lv_obj_create(lv_scr_act());
        if (_ball == nullptr)
        {
            cleanup_created();
            bsp_display_unlock();
            ESP_UTILS_LOGE("Create ball failed");
            return false;
        }
        lv_obj_set_size(_ball, kBallRadius * 2, kBallRadius * 2);
        lv_obj_set_style_radius(_ball, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_ball, lv_color_hex(0xD62828), 0);
        lv_obj_set_style_border_width(_ball, 0, 0);
        lv_obj_set_style_shadow_width(_ball, 3, 0);
        int center_x = _screen_area.x1 + _display_width / 2;
        int center_y = _screen_area.y1 + _display_height / 2;
        lv_obj_set_pos(_ball, center_x - kBallRadius, center_y - kBallRadius);
        lv_obj_add_flag(_ball, LV_OBJ_FLAG_HIDDEN);

        bsp_display_unlock();
        return true;
    }

    void Qmi8658Ball::calibration_task(void *arg)
    {
        Qmi8658Ball *app = static_cast<Qmi8658Ball *>(arg);
        app->perform_level_calibration();
    }

    bool Qmi8658Ball::init_calibration_button(void)
    {
        if (_calib_button_isr_registered)
        {
            return true;
        }

        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << kCalibButtonGpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE};
        esp_err_t ret = gpio_config(&io_conf);
        ESP_UTILS_CHECK_FALSE_RETURN(ret == ESP_OK, false, "Configure calibration GPIO failed: %s", esp_err_to_name(ret));

        ret = gpio_isr_handler_add(kCalibButtonGpio, button_isr_handler, this);
        if (ret == ESP_ERR_INVALID_STATE)
        {
            ret = gpio_install_isr_service(0);
            ESP_UTILS_CHECK_FALSE_RETURN(ret == ESP_OK, false,
                                         "Install GPIO ISR service failed: %s", esp_err_to_name(ret));
            ret = gpio_isr_handler_add(kCalibButtonGpio, button_isr_handler, this);
        }
        ESP_UTILS_CHECK_FALSE_RETURN(ret == ESP_OK, false, "Add calibration GPIO ISR failed: %s", esp_err_to_name(ret));
        _calib_button_isr_registered = true;
        return true;
    }

    void Qmi8658Ball::perform_level_calibration(void)
    {
        qmi8658_data_t data;
        constexpr int kCalibSamples = 200;
        float sum_x = 0.0f, sum_y = 0.0f;
        float max_x = -10.0f, min_x = 10.0f;
        float max_y = -10.0f, min_y = 10.0f;
        int valid_samples = 0;

        ESP_UTILS_LOGI("Starting level calibration...");

        if (_calibration_ui_update_timer == nullptr)
        {
            _calibration_ui_update_timer = lv_timer_create(calibration_ui_update_timer_cb, 100, this);
        }

        for (int i = 0; i < kCalibSamples && !_closing; i++)
        {
            if (qmi8658_read_sensor_data(_qmi8658_dev, &data) == ESP_OK)
            {
                sum_x += data.accelX;
                sum_y += data.accelY;
                valid_samples++;

                max_x = fmaxf(max_x, data.accelX);
                min_x = fminf(min_x, data.accelX);
                max_y = fmaxf(max_y, data.accelY);
                min_y = fminf(min_y, data.accelY);
            }

            if (i % 10 == 0)
            {
                _calib_progress = (i * 100) / kCalibSamples;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        float range_x = max_x - min_x;
        float range_y = max_y - min_y;

        if (_closing)
        {
            _calib_state = CALIB_IDLE;
            _calibration_done = false;
        }
        else if (valid_samples < (kCalibSamples * 8 / 10))
        {
            ESP_UTILS_LOGW("Calibration failed: only %d/%d valid samples", valid_samples, kCalibSamples);
            _calib_state = CALIB_FAILED;
            _calibration_done = false;
        }
        else if (range_x > 0.1f || range_y > 0.1f)
        {
            ESP_UTILS_LOGW("Calibration unstable (X: %.4f, Y: %.4f)", range_x, range_y);
            _calib_state = CALIB_FAILED;
            _calibration_done = false;
        }
        else
        {
            _accel_bias_x = sum_x / valid_samples;
            _accel_bias_y = sum_y / valid_samples;
            _calib_state = CALIB_DONE;
            _calibration_done = true;
            ESP_UTILS_LOGI("Calibration complete. Bias X: %.4f, Y: %.4f", _accel_bias_x, _accel_bias_y);
        }

        _calibration_task_handle = nullptr;
        vTaskDelete(NULL);
    }

    void Qmi8658Ball::update_calibration_ui(void)
    {
        esp_err_t lock_ret = bsp_display_lock(pdMS_TO_TICKS(50));
        if (lock_ret != ESP_OK)
        {
            ESP_UTILS_LOGW("Skip calibration UI update, display lock failed: %s", esp_err_to_name(lock_ret));
            return;
        }

        switch (_calib_state)
        {
        case CALIB_RUNNING:
            if (_calib_progress_bar)
            {
                lv_bar_set_value(_calib_progress_bar, _calib_progress, LV_ANIM_OFF);
            }
            if (_calib_progress_label)
            {
                lv_label_set_text_fmt(_calib_progress_label, "Calibrate %d%%", _calib_progress);
            }
            break;

        case CALIB_DONE:
            if (_calib_progress_label)
            {
                lv_label_set_text(_calib_progress_label, "Done");
            }

            if (_ball)
            {
                lv_obj_clear_flag(_ball, LV_OBJ_FLAG_HIDDEN);
            }
            if (_calib_label)
            {
                lv_obj_clear_flag(_calib_label, LV_OBJ_FLAG_HIDDEN);
            }

            if (_calib_progress_bar)
            {
                lv_obj_add_flag(_calib_progress_bar, LV_OBJ_FLAG_HIDDEN);
            }
            if (_calib_progress_label)
            {
                lv_obj_add_flag(_calib_progress_label, LV_OBJ_FLAG_HIDDEN);
            }

            if (!_update_task_handle && _ball)
            {
                BaseType_t task_created = xTaskCreatePinnedToCore(
                    ball_update_task,
                    "ball_update",
                    8192,
                    this,
                    3,
                    &_update_task_handle,
                    0);
                if (task_created != pdPASS)
                {
                    _update_task_handle = nullptr;
                    ESP_UTILS_LOGE("Create ball update task failed");
                }
            }
            if (_calibration_ui_update_timer)
            {
                lv_timer_del(_calibration_ui_update_timer);
                _calibration_ui_update_timer = nullptr;
            }
            break;

        case CALIB_FAILED:
            if (_calib_progress_label)
            {
                lv_label_set_text(_calib_progress_label, "Retry calibration");
            }
            bsp_display_unlock();
            _calib_state = CALIB_RUNNING;
            _calib_progress = 0;
            {
                BaseType_t task_created = xTaskCreatePinnedToCore(
                    calibration_task,
                    "calibration_retry",
                    4096,
                    this,
                    5,
                    &_calibration_task_handle,
                    0);
                if (task_created != pdPASS)
                {
                    _calibration_task_handle = nullptr;
                    _calib_state = CALIB_FAILED;
                    ESP_UTILS_LOGE("Create calibration retry task failed");
                }
            }
            return;

        default:
            break;
        }

        bsp_display_unlock();
    }

    void Qmi8658Ball::apply_calibration_and_deadzone(qmi8658_data_t *data)
    {
        if (_calibration_done)
        {
            data->accelX -= _accel_bias_x;
            data->accelY -= _accel_bias_y;

            if (fabsf(data->accelX) < kCalibrationDeadzone)
            {
                data->accelX = 0.0f;
            }
            if (fabsf(data->accelY) < kCalibrationDeadzone)
            {
                data->accelY = 0.0f;
            }
        }
    }

    void Qmi8658Ball::constrain_to_rounded_rect(int *x, int *y)
    {
        const int screen_width = _display_width;
        const int screen_height = _display_height;
        const int center_x = screen_width / 2;
        const int center_y = screen_height / 2;
        const float safe_radius = fmaxf(1.0f, (float)fminf(screen_width, screen_height) / 2.0f - kBallRadius - 2.0f);

        if (*x < kBallRadius)
        {
            *x = kBallRadius;
        }
        if (*x > screen_width - kBallRadius)
        {
            *x = screen_width - kBallRadius;
        }
        if (*y < kBallRadius)
        {
            *y = kBallRadius;
        }
        if (*y > screen_height - kBallRadius)
        {
            *y = screen_height - kBallRadius;
        }

        float dx = (float)(*x - center_x);
        float dy = (float)(*y - center_y);
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > safe_radius) {
            float ratio = safe_radius / dist;
            *x = center_x + (int)(dx * ratio);
            *y = center_y + (int)(dy * ratio);
        }
    }

    void IRAM_ATTR Qmi8658Ball::button_isr_handler(void *arg)
    {
        Qmi8658Ball *app = static_cast<Qmi8658Ball *>(arg);
        if (app)
        {
            app->_recalibration_requested = true;
            if (app->_update_task_handle != nullptr)
            {
                BaseType_t higher_priority_task_woken = pdFALSE;
                vTaskNotifyGiveFromISR(app->_update_task_handle, &higher_priority_task_woken);
                portYIELD_FROM_ISR(higher_priority_task_woken);
            }
        }
    }

    void Qmi8658Ball::ball_update_task(void *arg)
    {
        Qmi8658Ball *app = static_cast<Qmi8658Ball *>(arg);
        qmi8658_data_t data;

        while (!app->_closing && app->_calib_state != CALIB_DONE && app->_calib_state != CALIB_FAILED)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if (app->_closing || app->_calib_state != CALIB_DONE)
        {
            app->_update_task_handle = nullptr;
            vTaskDelete(NULL);
            return;
        }

        int rel_x = app->_display_width / 2;
        int rel_y = app->_display_height / 2;

        while (!app->_closing)
        {
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kTaskDelayMs));

            if (app->_recalibration_requested)
            {
                app->_recalibration_requested = false;

                esp_err_t lock_ret = bsp_display_lock(pdMS_TO_TICKS(50));
                if (lock_ret == ESP_OK)
                {
                    lv_obj_add_flag(app->_ball, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(app->_calib_progress_bar, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(app->_calib_progress_label, LV_OBJ_FLAG_HIDDEN);
                    bsp_display_unlock();
                }
                else
                {
                    ESP_UTILS_LOGW("Skip recalibration UI hide, display lock failed: %s", esp_err_to_name(lock_ret));
                }

                app->_calib_state = CALIB_RUNNING;
                app->_calib_progress = 0;
                BaseType_t task_created = xTaskCreatePinnedToCore(
                    calibration_task,
                    "calibration_recalib",
                    4096,
                    app,
                    5,
                    &app->_calibration_task_handle,
                    0);
                if (task_created != pdPASS)
                {
                    app->_calibration_task_handle = nullptr;
                    app->_calib_state = CALIB_FAILED;
                    ESP_UTILS_LOGE("Create recalibration task failed");
                    continue;
                }

                while (!app->_closing && app->_calib_state != CALIB_DONE && app->_calib_state != CALIB_FAILED)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                if (app->_closing || app->_calib_state != CALIB_DONE)
                {
                    continue;
                }

                lock_ret = bsp_display_lock(pdMS_TO_TICKS(50));
                if (lock_ret == ESP_OK)
                {
                    lv_obj_clear_flag(app->_ball, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(app->_calib_progress_bar, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(app->_calib_progress_label, LV_OBJ_FLAG_HIDDEN);
                    bsp_display_unlock();
                }
                else
                {
                    ESP_UTILS_LOGW("Skip recalibration UI show, display lock failed: %s", esp_err_to_name(lock_ret));
                }

                rel_x = app->_display_width / 2;
                rel_y = app->_display_height / 2;
                int center_x = app->_screen_area.x1 + rel_x;
                int center_y = app->_screen_area.y1 + rel_y;
                lock_ret = bsp_display_lock(pdMS_TO_TICKS(50));
                if (lock_ret == ESP_OK)
                {
                    lv_obj_set_pos(app->_ball, center_x - kBallRadius, center_y - kBallRadius);
                    bsp_display_unlock();
                }
            }

            bool ready;
            esp_err_t ret = qmi8658_is_data_ready(app->_qmi8658_dev, &ready);
            if (ret != ESP_OK || !ready)
            {
                continue;
            }

            ret = qmi8658_read_sensor_data(app->_qmi8658_dev, &data);
            if (ret == ESP_OK)
            {
                app->apply_calibration_and_deadzone(&data);

                int move_x = -(int)(data.accelY * kAccelScaleFactor);
                int move_y = (int)(data.accelX * kAccelScaleFactor);
                rel_x += move_x;
                rel_y += move_y;

                app->constrain_to_rounded_rect(&rel_x, &rel_y);

                int abs_x = app->_screen_area.x1 + rel_x;
                int abs_y = app->_screen_area.y1 + rel_y;

                esp_err_t lock_ret = bsp_display_lock(pdMS_TO_TICKS(50));
                if (lock_ret == ESP_OK)
                {
                    lv_obj_set_pos(app->_ball, abs_x - kBallRadius, abs_y - kBallRadius);
                    bsp_display_unlock();
                }
                else
                {
                    ESP_UTILS_LOGW("Skip ball update, display lock failed: %s", esp_err_to_name(lock_ret));
                }
            }

        }
        app->_update_task_handle = nullptr;
        vTaskDelete(NULL);
    }

    void Qmi8658Ball::calibration_ui_update_timer_cb(lv_timer_t *timer)
    {
        Qmi8658Ball *app = static_cast<Qmi8658Ball *>(timer->user_data);
        app->update_calibration_ui();
    }

} // namespace esp_brookesia::apps
