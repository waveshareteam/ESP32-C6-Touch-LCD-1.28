#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "nvs_flash.h"
#include "iot_button.h"
#include "button_gpio.h"

#include "esp_brookesia.hpp"
#include "boost/thread.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "esp_lib_utils.h"

#include "DrawingBoard.hpp"
#include "Qmi8658Ball.hpp"
#include "Settings.hpp"

#include "./dark/stylesheet.hpp"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;
using namespace esp_brookesia::apps;

constexpr bool EXAMPLE_SHOW_MEM_INFO = true;

extern "C" void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        ESP_UTILS_CHECK_FALSE_EXIT(ret == ESP_OK, "Erase NVS failed: %s", esp_err_to_name(ret));
        ret = nvs_flash_init();
    }
    ESP_UTILS_CHECK_FALSE_EXIT(ret == ESP_OK, "Init NVS failed: %s", esp_err_to_name(ret));

    esp_err_t sd_ret = bsp_sdcard_mount();
    if (sd_ret != ESP_OK) {
        ESP_UTILS_LOGW("SD card mount failed: %s", esp_err_to_name(sd_ret));
    }

    ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");
    lv_display_t *display = bsp_display_start();
    ESP_UTILS_CHECK_NULL_EXIT(display, "Start display failed");

    /* Configure GUI lock */
    LvLock::registerCallbacks([](int timeout_ms) {
        esp_err_t ret = bsp_display_lock(timeout_ms);
        ESP_UTILS_CHECK_FALSE_RETURN(ret == ESP_OK, false, "Lock failed (timeout_ms: %d)", timeout_ms);
        return true;
    }, []() {
        bsp_display_unlock();
        return true;
    });

    /* Create a phone object */
    Phone *phone = new (std::nothrow) Phone(display);
    ESP_UTILS_CHECK_NULL_EXIT(phone, "Create phone failed");

    // Load and activate the 240x240 dark theme for the round C6 display.
    Stylesheet *stylesheet = new systems::phone::Stylesheet(STYLESHEET_240_240_DARK);
    ESP_UTILS_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");
    ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
    ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");
    delete stylesheet;
    // ============================================================================

    {
        // Acquire lock before operating LVGL from non-GUI tasks
        LvLockGuard gui_guard;

        /* Begin the phone */
        ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");

        /* Initialize and install apps from the system registry (e.g., Settings, File Manager) */
        std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
        ESP_UTILS_CHECK_FALSE_EXIT(phone->initAppFromRegistry(inited_apps), "Init app registry failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installAppFromRegistry(inited_apps), "Install app registry failed");


        auto app_settings = esp_brookesia::apps::Settings::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app_settings), "Install Settings failed");


        auto drawing_board_app = esp_brookesia::apps::DrawingBoard::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(drawing_board_app), "Install Drawpanel failed");

        auto qmi_ball_app = esp_brookesia::apps::Qmi8658Ball::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(qmi_ball_app), "Install Qmiball failed");


        /* Create a timer to update the status bar clock every second */
        lv_timer_t *clock_timer = lv_timer_create([](lv_timer_t *t) {
            time_t now;
            struct tm timeinfo;
            Phone *phone = (Phone *)t->user_data;

            ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");

            time(&now);
            localtime_r(&now, &timeinfo);

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min),
                "Refresh status bar failed"
            );
        }, 1000, phone);
        ESP_UTILS_CHECK_NULL_EXIT(clock_timer, "Create status bar clock timer failed");
    }

    ESP_UTILS_CHECK_ERROR_EXIT(bsp_display_backlight_on(), "Turn on display backlight failed");

    if constexpr (EXAMPLE_SHOW_MEM_INFO) {
        esp_utils::thread_config_guard thread_config({
            .name = "mem_info",
            .stack_size = 4096,
        });
        boost::thread([ = ]() {
            size_t internal_free = 0;
            size_t internal_total = 0;

            while (1) {
                internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);

                {
                    LvLockGuard gui_guard;
                    ESP_UTILS_CHECK_FALSE_EXIT(
                        phone->getDisplay().getRecentsScreen()->setMemoryLabel(
                            internal_free / 1024, internal_total / 1024, 0, 0
                        ), "Set memory label failed"
                    );
                }

                boost::this_thread::sleep_for(boost::chrono::seconds(5));
            }
        }).detach();
    }
}
