#pragma once
#include "Board_Configuration.h"
#include "LCD_Driver.h"

#if defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
#include "Arduino.h"
#include <Wire.h>
#include "TCA9555PWR.h"

#define CST328_ADDR           0x1A
#define CST328_INT_PIN        -1
#define CST328_RST_PIN        -1


#define TOUCH_MAX_POINTS                        (5)      
/* CST328 registers */
#define ESP_LCD_TOUCH_CST328_READ_Number_REG    (0xD005)
#define ESP_LCD_TOUCH_CST328_READ_XY_REG        (0xD000)
#define ESP_LCD_TOUCH_CST328_READ_Checksum_REG  (0x80FF)
#define ESP_LCD_TOUCH_CST328_CONFIG_REG         (0x8047)

//debug info
/****************HYN_REG_MUT_DEBUG_INFO_MODE address start***********/
#define HYN_REG_MUT_DEBUG_INFO_IC_CHECKSUM      0xD208
#define HYN_REG_MUT_DEBUG_INFO_FW_VERSION       0xD204
#define HYN_REG_MUT_DEBUG_INFO_IC_TYPE			    0xD202
#define HYN_REG_MUT_DEBUG_INFO_PROJECT_ID			  0xD200 
#define HYN_REG_MUT_DEBUG_INFO_BOOT_TIME        0xD1FC
#define HYN_REG_MUT_DEBUG_INFO_RES_Y            0xD1FA
#define HYN_REG_MUT_DEBUG_INFO_RES_X            0xD1F8
#define HYN_REG_MUT_DEBUG_INFO_KEY_NUM          0xD1F7
#define HYN_REG_MUT_DEBUG_INFO_TP_NRX           0xD1F6
#define HYN_REG_MUT_DEBUG_INFO_TP_NTX           0xD1F4
//workmode
#define HYN_REG_MUT_DEBUG_INFO_MODE             0xD101
#define HYN_REG_MUT_RESET_MODE            		  0xD102
#define HYN_REG_MUT_DEBUG_RECALIBRATION_MODE    0xD104
#define HYN_REG_MUT_DEEP_SLEEP_MODE    			    0xD105
#define HYN_REG_MUT_DEBUG_POINT_MODE	    	    0xD108
#define HYN_REG_MUT_NORMAL_MODE                 0xD109

#define HYN_REG_MUT_DEBUG_RAWDATA_MODE          0xD10A
#define HYN_REG_MUT_DEBUG_DIFF_MODE             0xD10D
#define HYN_REG_MUT_DEBUG_FACTORY_MODE          0xD119
#define HYN_REG_MUT_DEBUG_FACTORY_MODE_2        0xD120

extern struct Touch_Data touch_data;

/* Touch GESTURE */
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
uint8_t CST328_Touch_Reset(void);
uint16_t CST328_Read_cfg(void);
uint8_t Touch_Read_Data(void);

#endif
