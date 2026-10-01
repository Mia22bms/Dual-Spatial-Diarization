#pragma once

#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "lvgl_music.h"

#ifdef __cplusplus
extern "C" {
#endif


void lv_example_1(void);
void lvgl_set_bat_volts(uint16_t bat_adc);
void lvgl_set_backlight_slider_value(uint16_t backlight);
void LVGL_Search_Music(void);

#ifdef __cplusplus
}
#endif
