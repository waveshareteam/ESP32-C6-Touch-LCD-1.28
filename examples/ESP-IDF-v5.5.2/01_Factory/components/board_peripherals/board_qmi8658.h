#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyr_x;
    int16_t gyr_y;
    int16_t gyr_z;
    float angle_x;
    float angle_y;
    float angle_z;
} board_qmi8658_data_t;

esp_err_t board_qmi8658_init(i2c_master_bus_handle_t bus_handle);
bool board_qmi8658_read_data(board_qmi8658_data_t *data);

#ifdef __cplusplus
}
#endif
