#pragma once
#include <Arduino.h> 

// LCD driver selection, remove front #, leaving only one driver enabled
// #define CONFIG_WAVESHARE_1_46INCH_TOUCH_LCD              1
// #define CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD              1
// #define CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD              1
// #define CONFIG_WAVESHARE_2INCH_TOUCH_LCD                 1
// #define CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD               1
 #define CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD               1

// LCD rotation Angle selection, remove the front #, leaving only one drive
// #define CONFIG_EXAMPLE_DISPLAY_ROTATION_0_DEGREE         1 
// #define CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE        1
// #define CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE       1
#define CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE       1



#if defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD) || defined(CONFIG_WAVESHARE_1_46INCH_TOUCH_LCD)
  #define CONFIG_Round_LCD_screen 1
#endif


#if defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD) || defined(CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD) || defined(CONFIG_WAVESHARE_1_46INCH_TOUCH_LCD) || defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
  #define CONFIG_LVGL_COLOR_16_SWAP 1
#endif



#if defined(CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD)
  #define TouchPad_swap_xy     0
  #define TouchPad_mirror_x    1
  #define TouchPad_mirror_y    0
#endif
#if defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
  #define TouchPad_swap_xy     0
  #define TouchPad_mirror_x    0
  #define TouchPad_mirror_y    0
#endif
#if defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
  #define TouchPad_swap_xy     0
  #define TouchPad_mirror_x    0
  #define TouchPad_mirror_y    0
#endif
#if defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
  #define TouchPad_swap_xy     0
  #define TouchPad_mirror_x    0
  #define TouchPad_mirror_y    0
#endif
/*************************************************************************************************************************************************************************************************************************
**************************************************************************************************************************************************************************************************************************/
#ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
  #define Touch_swap_xy     (!TouchPad_swap_xy)
  #define Touch_mirror_x    (!TouchPad_mirror_x)
  #define Touch_mirror_y    (TouchPad_mirror_y)
#elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE)
  #define Touch_swap_xy     (TouchPad_swap_xy)
  #define Touch_mirror_x    (!TouchPad_mirror_x)
  #define Touch_mirror_y    (!TouchPad_mirror_y)
#elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
  #define Touch_swap_xy     (!TouchPad_swap_xy)
  #define Touch_mirror_x    (TouchPad_mirror_x)
  #define Touch_mirror_y    (!TouchPad_mirror_y)
#else
  #define Touch_swap_xy     (TouchPad_swap_xy)
  #define Touch_mirror_x    (TouchPad_mirror_x)
  #define Touch_mirror_y    (TouchPad_mirror_y)
#endif