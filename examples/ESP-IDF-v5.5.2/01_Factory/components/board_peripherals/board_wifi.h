#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_wifi_init(void);
esp_err_t board_wifi_connect_default(void);
bool board_wifi_scan(wifi_ap_record_t *ap_info, uint16_t *scan_number, uint16_t scan_max_num);
void board_wifi_get_ip(char *ip);

#ifdef __cplusplus
}
#endif
