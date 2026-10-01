#pragma once

#include "sdkconfig.h"
#ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
#include "esp_lcd_touch_axs5106.h"
#elif defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD)
#include "CST816.h"
#elif defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
#include "CST816.h"
#elif defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
#include "CST328.h"
#elif defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
#include "esp_lcd_touch_ft6336.h"
#else
//#warning "LCD Configuration is not properly defined"
//#error "No valid touch screen configuration is defined"
#endif



#ifdef __cplusplus
extern "C" {
#endif

extern esp_lcd_touch_handle_t tp_handle;

void touch_driver_init(void);

#ifdef __cplusplus
}
#endif
