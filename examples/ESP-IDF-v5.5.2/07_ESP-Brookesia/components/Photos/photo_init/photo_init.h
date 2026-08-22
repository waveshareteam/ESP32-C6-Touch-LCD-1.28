#pragma once

#include "esp_log.h"
#include "bsp/esp-bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char **list;
    int count;
} photo_file_list_t;

/**
 * @brief Get a file list by extension.
 *
 * @param[in] dir_path Directory path to scan.
 * @param[in] extension File extension, for example ".jpg".
 * @param[out] out Output list. Release it with photo_file_list_free().
 * @return ESP_OK on success, or an ESP-IDF error code.
 */
esp_err_t photo_get_file_list_by_ext(const char *dir_path, const char *extension, photo_file_list_t *out);

/**
 * @brief Free a file list returned by photo_get_file_list_by_ext().
 *
 * @param[in,out] list File list to release.
 */
void photo_file_list_free(photo_file_list_t *list);

/**
 * @brief Initialize the JPEG photo decoder.
 *
 * @param disp LVGL display handle used for refresh.
 */
void init_photo_player(lv_display_t *disp);

/**
 * @brief Create the photo browser UI.
 *
 * @param parent Parent LVGL object.
 * @param file_list JPG file list owned by the caller.
 */
void init_photo_ui_screen(lv_obj_t *parent, photo_file_list_t *file_list);

/**
 * @brief Release decoder buffers and photo UI objects.
 */
void deinit_photo_player(void);

#ifdef __cplusplus
}
#endif
