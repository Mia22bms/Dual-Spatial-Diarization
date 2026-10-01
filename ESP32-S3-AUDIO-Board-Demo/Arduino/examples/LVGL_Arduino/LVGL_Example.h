#pragma once

#include "LVGL_Driver.h"
#include "LCD_Driver.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "Wireless.h"
#include "BAT_Driver.h"

#define EXAMPLE1_LVGL_TICK_PERIOD_MS  1000

extern lv_timer_t * auto_step_timer;

void Backlight_adjustment_event_cb(lv_event_t * e);
void Lvgl_Example1(void);
void LVGL_Backlight_adjustment(uint8_t Backlight);