#pragma once
#include "Board_Configuration.h"
#include "LCD_Driver.h"

#if defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD) || defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
#include <Arduino.h>
#include <Wire.h>
#include "TCA9555PWR.h"

#define CST816_ADDR             0x15
#define CST816_INT_PIN          -1
#define CST816_RST_PIN          -1                      // EXIO1


#define TOUCH_MAX_POINTS             (1)    
/* CST816 GESTURE */
//debug info
/****************HYN_REG_MUT_DEBUG_INFO_MODE address start***********/
#define CST816_REG_GestureID      0x01
#define CST816_REG_Version        0x15
#define CST816_REG_ChipID         0xA7
#define CST816_REG_ProjID         0xA8
#define CST816_REG_FwVersion      0xA9
#define CST816_REG_AutoSleepTime  0xF9
#define CST816_REG_DisAutoSleep   0xFE


extern struct Touch_Data touch_data;

enum GESTURE {
  NONE = 0x00,
  SWIPE_UP = 0x01,
  SWIPE_DOWN = 0x02,
  SWIPE_LEFT = 0x03,
  SWIPE_RIGHT = 0x04,
  SINGLE_CLICK = 0x05,
  DOUBLE_CLICK = 0x0B,
  LONG_PRESS = 0x0C
};
struct Touch_Data{
  uint8_t points;    // Number of touch points
  struct {
    uint16_t x; /*!< X coordinate */
    uint16_t y; /*!< Y coordinate */
    uint16_t strength; /*!< Strength */
  }coords[TOUCH_MAX_POINTS];
  GESTURE gesture;    /*!< uint8_t */
};

uint8_t Touch_Init();
void Touch_Loop(void);
uint8_t CST816_Touch_Reset(void);
void CST816_AutoSleep(bool Sleep_State);
uint16_t CST816_Read_cfg(void);
String Touch_GestureName(void);
uint8_t Touch_Read_Data(void);

#endif