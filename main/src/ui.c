#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "board_pins.h"
#include "ui.h"

static const char *TAG = "ui";

static lv_obj_t *s_map_img = NULL;
static lv_obj_t *s_satellite_dots[UI_MAX_SATS] = {0};

// Image generated from main/images/world_480x320.png via lvgl_port_create_c_image
LV_IMG_DECLARE(world_480x320);

static void map_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code != LV_EVENT_PRESSED && code != LV_EVENT_RELEASED && code != LV_EVENT_PRESSING && code != LV_EVENT_CLICKED) {
        return;
    }

    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);
    ESP_LOGI(TAG, "Map touch (code=%d): x=%d y=%d", (int)code, (int)p.x, (int)p.y);
}

static lv_obj_t *create_sat_dot(lv_obj_t *parent, lv_color_t color, lv_color_t border_color) {
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 2, 0);
    lv_obj_set_style_border_color(dot, border_color, 0);
    lv_obj_move_foreground(dot);
    return dot;
}

static void create_main_screen(void) {
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    s_map_img = lv_img_create(scr);
    lv_img_set_src(s_map_img, &world_480x320);
    lv_obj_set_size(s_map_img, LCD_H_RES, LCD_V_RES);
    lv_obj_align(s_map_img, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_flag(s_map_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_map_img, map_touch_cb, LV_EVENT_ALL, NULL);

    s_satellite_dots[0] = create_sat_dot(s_map_img, lv_color_hex(0xFF0000), lv_color_hex(0xFFFFFF));
    s_satellite_dots[1] = create_sat_dot(s_map_img, lv_color_hex(0x00FF00), lv_color_hex(0x000000));
    s_satellite_dots[2] = create_sat_dot(s_map_img, lv_color_hex(0x0000FF), lv_color_hex(0xFFFFFF));

    lv_coord_t w = lv_obj_get_width(s_map_img);
    lv_coord_t h = lv_obj_get_height(s_map_img);

    for (int i = 0; i < UI_MAX_SATS; ++i) {
        if (!s_satellite_dots[i])
            continue;
        lv_coord_t dot_w = lv_obj_get_width(s_satellite_dots[i]);
        lv_coord_t dot_h = lv_obj_get_height(s_satellite_dots[i]);
        lv_coord_t center_x = (w - dot_w) / 2;
        lv_coord_t center_y = (h - dot_h) / 2;
        lv_obj_set_pos(s_satellite_dots[i], center_x, center_y);
    }
}

void ui_init(void) {
    ESP_LOGI(TAG, "Creating UI");
    lvgl_port_lock(0);
    create_main_screen();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "UI initialized");
}

void ui_set_satellite_pixel(int sat_index, int x_px, int y_px) {
    if (sat_index < 0 || sat_index >= UI_MAX_SATS)
        return;

    lvgl_port_lock(0);

    if (!s_map_img) {
        lvgl_port_unlock();
        return;
    }

    lv_obj_t *dot = s_satellite_dots[sat_index];
    if (!dot) {
        lvgl_port_unlock();
        return;
    }

    lv_coord_t map_w = lv_obj_get_width(s_map_img);
    lv_coord_t map_h = lv_obj_get_height(s_map_img);
    lv_coord_t dot_w = lv_obj_get_width(dot);
    lv_coord_t dot_h = lv_obj_get_height(dot);

    if (x_px < 0)
        x_px = 0;
    if (x_px >= map_w)
        x_px = map_w - 1;
    if (y_px < 0)
        y_px = 0;
    if (y_px >= map_h)
        y_px = map_h - 1;

    lv_coord_t pos_x = (lv_coord_t)x_px - dot_w / 2;
    lv_coord_t pos_y = (lv_coord_t)y_px - dot_h / 2;

    if (pos_x < 0)
        pos_x = 0;
    if (pos_x > map_w - dot_w)
        pos_x = map_w - dot_w;
    if (pos_y < 0)
        pos_y = 0;
    if (pos_y > map_h - dot_h)
        pos_y = map_h - dot_h;

    lv_obj_set_pos(dot, pos_x, pos_y);

    lvgl_port_unlock();
}
