#include "Touch_ft6336.h"
#include <Wire.h>
#include "driver/i2c.h"


#if defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)

TouchDrvFT6X36 touch;

/* Reset controller */
static uint8_t Touch_Reset(void)
{
  Set_EXIO(TCA9555_EXIO1,LOW);
  vTaskDelay(pdMS_TO_TICKS(100));
  Set_EXIO(TCA9555_EXIO1,HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  return true;
}

uint8_t Touch_Init(void) 
{
  Touch_Reset();
  touch.begin(Wire, FT6X36_SLAVE_ADDRESS);
  return true;
}


#endif