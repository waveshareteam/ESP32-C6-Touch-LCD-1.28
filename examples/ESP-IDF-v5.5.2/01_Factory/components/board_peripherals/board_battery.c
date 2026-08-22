#include "board_battery.h"

#include "bsp/esp-bsp.h"
#include "esp_check.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define BOARD_BATTERY_ADC_ATTEN ADC_ATTEN_DB_12
#define BOARD_BATTERY_DIVIDER_RATIO 3.0f

static const char *TAG = "board_battery";
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static adc_unit_t adc_unit;
static adc_channel_t adc_channel;
static bool initialized;
static bool calibrated;

static bool battery_adc_calibration_init(adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t curve_config = {
        .unit_id = adc_unit,
        .chan = adc_channel,
        .atten = BOARD_BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&curve_config, &handle);
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (ret != ESP_OK)
    {
        adc_cali_line_fitting_config_t line_config = {
            .unit_id = adc_unit,
            .atten = BOARD_BATTERY_ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&line_config, &handle);
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "ADC calibration enabled on GPIO%d", BSP_BATTERY_ADC);
        return true;
    }

    ESP_LOGW(TAG, "ADC calibration unavailable, raw battery ADC will still be shown");
    return false;
}

esp_err_t board_battery_init(void)
{
    if (initialized)
    {
        return ESP_OK;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_io_to_channel(BSP_BATTERY_ADC, &adc_unit, &adc_channel),
                        TAG, "Battery GPIO to ADC channel failed");
    init_config.unit_id = adc_unit;

    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&init_config, &adc_handle), TAG, "ADC unit init failed");

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = BOARD_BATTERY_ADC_ATTEN,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(adc_handle, adc_channel, &chan_config),
                        TAG, "ADC channel init failed");

    calibrated = battery_adc_calibration_init(&cali_handle);
    initialized = true;
    return ESP_OK;
}

esp_err_t board_battery_read(uint16_t *adc_raw, float *voltage)
{
    int raw = 0;
    int voltage_mv = 0;

    if (!initialized)
    {
        ESP_RETURN_ON_ERROR(board_battery_init(), TAG, "Battery ADC init failed");
    }

    ESP_RETURN_ON_ERROR(adc_oneshot_read(adc_handle, adc_channel, &raw), TAG, "ADC read failed");
    if (adc_raw)
    {
        *adc_raw = (uint16_t)raw;
    }

    if (voltage)
    {
        if (calibrated && adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv) == ESP_OK)
        {
            *voltage = (voltage_mv / 1000.0f) * BOARD_BATTERY_DIVIDER_RATIO;
        }
        else
        {
            *voltage = ((float)raw / 4095.0f) * 3.3f * BOARD_BATTERY_DIVIDER_RATIO;
        }
    }

    return ESP_OK;
}
