#pragma once

#include <stdbool.h>
#include <time.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_pcf85063_init(i2c_master_bus_handle_t bus_handle);
bool board_pcf85063_get_time(struct tm *now_tm);
void board_pcf85063_set_time(struct tm *now_tm);

#ifdef __cplusplus
}
#endif
