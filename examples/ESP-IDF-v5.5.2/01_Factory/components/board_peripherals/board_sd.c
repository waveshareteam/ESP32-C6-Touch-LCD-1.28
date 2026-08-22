#include "board_sd.h"

#include <stdbool.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"

static const char *TAG = "board_sd";
static bool mounted;
static esp_err_t mount_result = ESP_ERR_INVALID_STATE;

esp_err_t board_sd_init(void)
{
    if (mounted)
    {
        return ESP_OK;
    }

    mount_result = bsp_sdcard_mount();
    if (mount_result == ESP_OK)
    {
        mounted = true;
        ESP_LOGI(TAG, "SD card mounted");
    }
    else
    {
        ESP_LOGW(TAG, "SD card mount failed: %s", esp_err_to_name(mount_result));
    }

    return mount_result;
}

esp_err_t board_sd_get_capacity(uint64_t *total_bytes)
{
    if (!total_bytes)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!mounted || !bsp_sdcard)
    {
        return mount_result == ESP_OK ? ESP_ERR_INVALID_STATE : mount_result;
    }

    *total_bytes = (uint64_t)bsp_sdcard->csd.capacity * bsp_sdcard->csd.sector_size;
    return ESP_OK;
}
