#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <assert.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "board_pins.h"
#include "display.h"
#include "ui.h"

static const char *TAG = "ui";

static lv_disp_t *s_disp = NULL;
static lv_indev_t *s_touch_indev = NULL;
static lv_obj_t *s_satellite_dot = NULL;

// Image generated from main/images/world_380x240.png  via lvgl_port_create_c_image in CMakeLists.txt
LV_IMG_DECLARE(world_380x240);

static void sat_anim_x_cb(void *obj, int32_t x) {
    lv_coord_t y = lv_obj_get_y_aligned(obj);
    lv_obj_set_pos(obj, (lv_coord_t)x, y);
}

static void start_satellite_animation(void) // Example for movement from lvlg examples, this will be orbit/tracking in next verspion. 
{
    if (!s_satellite_dot) {
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_var(&a, s_satellite_dot);
    lv_anim_set_exec_cb(&a, sat_anim_x_cb);

    lv_coord_t start_x = 0;
    lv_coord_t end_x = 380 - 10;

    lv_anim_set_values(&a, start_x, end_x);
    lv_anim_set_duration(&a, 12000);          // 12s one way
    lv_anim_set_playback_duration(&a, 12000); // and back
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_start(&a);
}

static void create_main_screen(void) {
    lv_obj_t *scr = lv_disp_get_scr_act(s_disp);

    lv_obj_t *map_img = lv_img_create(scr);
    lv_img_set_src(map_img, &world_380x240);

    lv_coord_t w = lv_disp_get_hor_res(s_disp);
    lv_coord_t h = lv_disp_get_ver_res(s_disp);
    lv_obj_set_size(map_img, w, h);
    lv_obj_center(map_img);

    s_satellite_dot = lv_obj_create(map_img);
    lv_obj_remove_style_all(s_satellite_dot);
    lv_obj_set_size(s_satellite_dot, 10, 10);
    lv_obj_set_style_radius(s_satellite_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_satellite_dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(s_satellite_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_satellite_dot, 2, 0);
    lv_obj_set_style_border_color(s_satellite_dot, lv_color_hex(0xFFFFFF), 0);

    lv_obj_set_pos(s_satellite_dot, w / 2, h / 2);

    start_satellite_animation();
}

void ui_init(display_t *display) {
    ESP_LOGI(TAG, "Initializing LVGL port");

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    lvgl_port_display_cfg_t disp_cfg = (lvgl_port_display_cfg_t){0};

    disp_cfg.io_handle = display->io;
    disp_cfg.panel_handle = display->panel;
    disp_cfg.buffer_size = LCD_H_RES * 40; // partial buffer in pixels
    disp_cfg.double_buffer = true;
    disp_cfg.hres = LCD_H_RES;
    disp_cfg.vres = LCD_V_RES;
    disp_cfg.monochrome = false;

    // Keep using LVGL rotation (software) consistent with what we see working now
    disp_cfg.rotation.swap_xy = true;
    disp_cfg.rotation.mirror_x = true;
    disp_cfg.rotation.mirror_y = false;

    disp_cfg.flags.buff_dma = true;
    // disp_cfg.flags.swap_bytes = true; // removed to fix color issues

    s_disp = lvgl_port_add_disp(&disp_cfg);
    assert(s_disp != NULL);

    lv_disp_set_default(s_disp);

    if (display->touch) {
        lvgl_port_touch_cfg_t touch_cfg = (lvgl_port_touch_cfg_t){0};
        touch_cfg.disp = s_disp;
        touch_cfg.handle = display->touch;

        s_touch_indev = lvgl_port_add_touch(&touch_cfg);
        assert(s_touch_indev != NULL);
    } else {
        ESP_LOGW(TAG, "Touch handle is NULL, LVGL will run without touch input");
    }

    lvgl_port_lock(0);
    create_main_screen();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "UI initialized");
}
