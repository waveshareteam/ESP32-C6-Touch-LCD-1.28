#ifndef __LVGL_UI_H__
#define __LVGL_UI_H__

#include <stdio.h>
#include "lvgl.h"


#ifdef __cplusplus
extern "C" {
#endif

void lvgl_ui_init(void);
void lvgl_ui_next_page(void);
void lvgl_ui_request_next_page(void);

#ifdef __cplusplus
}
#endif

#endif
