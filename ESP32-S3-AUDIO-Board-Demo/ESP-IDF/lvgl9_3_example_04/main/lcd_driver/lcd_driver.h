#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "sdkconfig.h"
#ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
#include "esp_lcd_jd9853.h"
#elif defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
#include "esp_lcd_st7796.h"
#endif




#ifdef __cplusplus
extern "C" {
#endif


 /* LCD IO and panel */
extern esp_lcd_panel_io_handle_t lcd_io;
extern esp_lcd_panel_handle_t lcd_panel;
//extern esp_lcd_touch_handle_t touch_handle = NULL;

esp_err_t lcd_driver_init(void);
void Set_Backlight(uint8_t Light);                   // Call this function to adjust the brightness of the backlight. The value of the parameter Light ranges from 0 to 100
uint8_t Read_Backlight_value(void);

#ifdef __cplusplus
}
#endif
