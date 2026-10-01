#pragma once
#include "Board_Configuration.h"
#include "LCD_Driver.h"


#if defined(CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD)

#include "Arduino.h"
#include <Wire.h>
#include "TCA9555PWR.h"

#define AXS5106_ADDR           0x63
#define AXS5106_INT_PIN        -1
#define AXS5106_RST_PIN        -1                      // EXIO1


#define TOUCH_MAX_POINTS             (2)    
/* AXS5106 GESTURE */
//debug info
/****************HYN_REG_MUT_DEBUG_INFO_MODE address start***********/
#define TOUCH_AXS5106_TOUCH_POINTS_REG (0X01)
#define TOUCH_AXS5106_TOUCH_P1_XH_REG (0x03)
#define TOUCH_AXS5106_TOUCH_P1_XL_REG (0x04)
#define TOUCH_AXS5106_TOUCH_P1_YH_REG (0x05)
#define TOUCH_AXS5106_TOUCH_P1_YL_REG (0x06)

#define TOUCH_AXS5106_TOUCH_P2_XH_REG (0x09)
#define TOUCH_AXS5106_TOUCH_P2_XL_REG (0x0A)
#define TOUCH_AXS5106_TOUCH_P2_YH_REG (0x0B)
#define TOUCH_AXS5106_TOUCH_P2_YL_REG (0x0C)

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
uint8_t AXS5106_Touch_Reset(void);
uint8_t Touch_Read_Data(void);

#endif