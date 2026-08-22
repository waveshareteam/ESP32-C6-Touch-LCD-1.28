#include "Settings.hpp"

#include "assets/app_setting_assets.h"
#include "esp_brookesia.hpp"
#include "esp_lib_utils.h"
#include "ui/settings_ui.h"
#include "ui/set_wifi_service.h"

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Settings"

LV_IMG_DECLARE(img_app_settings);

#define APP_NAME "Settings"

namespace esp_brookesia::apps {

Settings *Settings::_instance = nullptr;

Settings *Settings::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new Settings(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

Settings::Settings(bool use_status_bar, bool use_navigation_bar):
    App(APP_NAME, &img_app_settings, true, use_status_bar, use_navigation_bar)
{
}

Settings::~Settings()
{
    _instance = nullptr;
}

bool Settings::run(void)
{
    settings_ui_screen_init(closeFromUi);
    return true;
}

bool Settings::back(void)
{
    ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
    return true;
}

bool Settings::close(void)
{
    settings_ui_screen_deinit();
    return true;
}

bool Settings::init(void)
{
    esp_err_t err = wifi_c5_init_and_connect();
    ESP_UTILS_CHECK_FALSE_RETURN(err == ESP_OK, false, "WiFi init failed: %s", esp_err_to_name(err));
    return true;
}

bool Settings::deinit(void)
{
    return true;
}

bool Settings::pause(void)
{
    return true;
}

bool Settings::resume(void)
{
    return true;
}

void Settings::closeFromUi(void)
{
    if (_instance != nullptr) {
        _instance->notifyCoreClosed();
    }
}

} // namespace esp_brookesia::apps
