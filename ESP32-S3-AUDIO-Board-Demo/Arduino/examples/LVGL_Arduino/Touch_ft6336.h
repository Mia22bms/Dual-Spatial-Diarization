#pragma once
#include "Board_Configuration.h"
#include "LCD_Driver.h"
#include "TouchDrvFT6X36.hpp"

#if defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
#include <Arduino.h>

#include "TCA9555PWR.h"



extern TouchDrvFT6X36 touch;
uint8_t Touch_Init(void);

#endif