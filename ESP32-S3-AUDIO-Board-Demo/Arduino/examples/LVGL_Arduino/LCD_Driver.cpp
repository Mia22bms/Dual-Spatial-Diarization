#include "LCD_Driver.h"

void LCD_INIT() {  
  LCD_Init();
  TOUCH_INIT();
  Backlight_Init();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Backlight program


uint8_t LCD_Backlight = 90;
// backlight
void Backlight_Init()
{
  ledcAttach(LCD_Backlight_PIN, Frequency, Resolution);   
  ledcWrite(LCD_Backlight_PIN, Dutyfactor);  
  Set_Backlight(LCD_Backlight);      //0~100                 
}

void Set_Backlight(uint8_t Light)                     
{
  if(Light > Backlight_MAX || Light < 0)
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
  else{
    uint32_t Backlight = Light*10;
    if(Backlight == 1000)
      Backlight = 1024;
    ledcWrite(LCD_Backlight_PIN, Backlight);
  }
}