#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <assert.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "board_pins.h"
#include "ui.h"

static const char *TAG = "ui";

static lv_obj_t *s_satellite_dot = NULL;

// Image generated from main/images/world_480x320.png via lvgl_port_create_c_image
LV_IMG_DECLARE(world_480x320);

static void map_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code != LV_EVENT_POINTER_DOWN && code != LV_EVENT_POINTER_UP && code != LV_EVENT_PRESSING && code != LV_EVENT_CLICKED) {
        return;
    }

    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev) {
        return;
    }

    lv_point_t p;
    lv_indev_get_point(indev, &p);
    ESP_LOGI(TAG, "Map touch (code=%d): x=%d y=%d", (int)code, (int)p.x, (int)p.y);
}

static void sat_anim_x_cb(void *obj, int32_t x) {
    lv_coord_t y = lv_obj_get_y(obj);
    lv_obj_set_pos(obj, (lv_coord_t)x, y);
}

static void start_satellite_animation(void) {
    if (!s_satellite_dot) {
        return;
    }

    lv_obj_t *parent = lv_obj_get_parent(s_satellite_dot);
    lv_coord_t parent_w = lv_obj_get_width(parent);
    lv_coord_t dot_w = lv_obj_get_width(s_satellite_dot);

    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_var(&a, s_satellite_dot);
    lv_anim_set_exec_cb(&a, sat_anim_x_cb);

    lv_coord_t start_x = 0;
    lv_coord_t end_x = parent_w - dot_w;

    lv_anim_set_values(&a, start_x, end_x);
    lv_anim_set_duration(&a, 12000);
    lv_anim_set_playback_duration(&a, 12000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_start(&a);
}

static void create_main_screen(void) {
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    lv_obj_t *map_img = lv_img_create(scr);
    lv_img_set_src(map_img, &world_480x320);
    lv_obj_set_size(map_img, LCD_H_RES, LCD_V_RES);
    lv_obj_align(map_img, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_flag(map_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(map_img, map_touch_cb, LV_EVENT_ALL, NULL);

    s_satellite_dot = lv_obj_create(map_img);
    lv_obj_remove_style_all(s_satellite_dot);
    lv_obj_set_size(s_satellite_dot, 12, 12);
    lv_obj_set_style_radius(s_satellite_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_satellite_dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(s_satellite_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_satellite_dot, 2, 0);
    lv_obj_set_style_border_color(s_satellite_dot, lv_color_hex(0xFFFFFF), 0);
    lv_obj_move_foreground(s_satellite_dot);

    lv_coord_t w = lv_obj_get_width(map_img);
    lv_coord_t h = lv_obj_get_height(map_img);
    lv_coord_t dot_w = lv_obj_get_width(s_satellite_dot);
    lv_coord_t dot_h = lv_obj_get_height(s_satellite_dot);

    lv_coord_t center_x = (w - dot_w) / 2;
    lv_coord_t center_y = (h - dot_h) / 2;

    lv_obj_set_pos(s_satellite_dot, center_x, center_y);

    start_satellite_animation();
}

void ui_init(void) {
    ESP_LOGI(TAG, "Creating UI");

    lvgl_port_lock(0);
    create_main_screen();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "UI initialized");
}
