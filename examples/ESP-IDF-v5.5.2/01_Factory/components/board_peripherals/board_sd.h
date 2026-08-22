#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_sd_init(void);
esp_err_t board_sd_get_capacity(uint64_t *total_bytes);

#ifdef __cplusplus
}
#endif
