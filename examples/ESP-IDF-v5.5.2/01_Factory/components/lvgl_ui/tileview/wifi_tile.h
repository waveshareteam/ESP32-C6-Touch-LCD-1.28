#ifndef __WIFI_TILE_H__
#define __WIFI_TILE_H__

#include "../lvgl_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_tile_init(lv_obj_t *parent);
void wifi_tile_on_enter(void);

#ifdef __cplusplus
}
#endif

#endif
