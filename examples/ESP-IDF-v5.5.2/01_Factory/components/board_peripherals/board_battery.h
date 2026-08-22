#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_battery_init(void);
esp_err_t board_battery_read(uint16_t *adc_raw, float *voltage);

#ifdef __cplusplus
}
#endif
