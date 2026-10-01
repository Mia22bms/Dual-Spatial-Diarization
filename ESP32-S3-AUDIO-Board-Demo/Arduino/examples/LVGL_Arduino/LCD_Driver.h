#pragma once
#include "Board_Configuration.h"

#ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
#include "Display_JD9853.h"
#elif defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD)
#include "Display_ST77916.h"
#elif defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
#include "Display_ST7789.h"
#elif defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
#include "Display_ST7789.h"
#elif defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
#include "Display_ST7796.h"
#else
#warning "LCD Configuration is not properly defined"
#error "No valid touch screen configuration is defined"
#endif

#include "Touch_Driver.h"
#include "driver/ledc.h"

#define LCD_Backlight_PIN   5
#define PWM_Channel     1       // PWM Channel   
#define Frequency       20000   // PWM frequencyconst        
#define Resolution      10       // PWM resolution ratio     MAX:13
#define Dutyfactor      500     // PWM Dutyfactor      
#define Backlight_MAX   100

extern uint8_t LCD_Backlight;
void LCD_INIT();


void Backlight_Init(void);                             // Initialize the LCD backlight, which has been called in the LCD_Init function, ignore it                                                         
void Set_Backlight(uint8_t Light);                   // Call this function to adjust the brightness of the backlight. The value of the parameter Light ranges from 0 to 100
