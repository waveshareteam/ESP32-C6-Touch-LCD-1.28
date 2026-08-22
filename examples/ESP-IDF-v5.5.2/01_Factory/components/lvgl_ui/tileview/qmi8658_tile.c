#include "qmi8658_tile.h"

#include "board_qmi8658.h"

static lv_obj_t *label_accel_x;
static lv_obj_t *label_accel_y;
static lv_obj_t *label_accel_z;
static lv_obj_t *label_gyro_x;
static lv_obj_t *label_gyro_y;
static lv_obj_t *label_gyro_z;

static void compact_list_item(lv_obj_t *item)
{
    lv_obj_set_height(item, 28);
    lv_obj_set_style_pad_top(item, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(item, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_left(item, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_right(item, 18, LV_PART_MAIN);
}

static void qmi8658_time_cb(lv_timer_t *timer)
{
    board_qmi8658_data_t qmi_data;

    if (board_qmi8658_read_data(&qmi_data))
    {
        lv_label_set_text_fmt(label_accel_x, "%d", qmi_data.acc_x);
        lv_label_set_text_fmt(label_accel_y, "%d", qmi_data.acc_y);
        lv_label_set_text_fmt(label_accel_z, "%d", qmi_data.acc_z);
        lv_label_set_text_fmt(label_gyro_x, "%d", qmi_data.gyr_x);
        lv_label_set_text_fmt(label_gyro_y, "%d", qmi_data.gyr_y);
        lv_label_set_text_fmt(label_gyro_z, "%d", qmi_data.gyr_z);
    }
}

void qmi8658_tile_init(lv_obj_t *parent)
{
    lv_obj_t *list = lv_list_create(parent);
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(label, "QMI8658");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 3);

    lv_obj_set_size(list, lv_pct(86), 178);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 34);
    lv_obj_set_style_pad_row(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(list, 0, LV_PART_MAIN);

    lv_obj_t *list_item;
    list_item = lv_list_add_btn(list, NULL, "Accel X");
    compact_list_item(list_item);
    label_accel_x = lv_label_create(list_item);
    lv_label_set_text(label_accel_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Accel Y");
    compact_list_item(list_item);
    label_accel_y = lv_label_create(list_item);
    lv_label_set_text(label_accel_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Accel Z");
    compact_list_item(list_item);
    label_accel_z = lv_label_create(list_item);
    lv_label_set_text(label_accel_z, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro X");
    compact_list_item(list_item);
    label_gyro_x = lv_label_create(list_item);
    lv_label_set_text(label_gyro_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro Y");
    compact_list_item(list_item);
    label_gyro_y = lv_label_create(list_item);
    lv_label_set_text(label_gyro_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro Z");
    compact_list_item(list_item);
    label_gyro_z = lv_label_create(list_item);
    lv_label_set_text(label_gyro_z, "----");

    lv_timer_create(qmi8658_time_cb, 100, NULL);
}
