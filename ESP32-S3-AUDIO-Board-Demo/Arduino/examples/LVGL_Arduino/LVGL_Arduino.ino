/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include "LCD_Driver.h"
#include "Audio_ES8311.h"
#include "RTC_PCF85063.h"
#include "LVGL_Driver.h"
#include "MIC_MSM.h"
#include "SD_Card.h"
#include "LVGL_Example.h"
#include "BAT_Driver.h"
#include "I2S_Driver.h"
#include "Camera_Driver.h"
#include "Button_Driver.h"

void Driver_Loop(void *parameter)
{
  while(1)
  {
    PCF85063_Loop();
    BAT_Get_Volts();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
void Driver_Init()
{
  Flash_test();
  BAT_Init();
  I2C_Init();
  TCA9555PWR_Init(0x0000);   
  Backlight_Init();
  PCF85063_Init();
  SD_Init();
  Button_Init();
}
void setup()
{
  Driver_Init();

  MIC_Init();
  Audio_Init();
  LCD_INIT();
  Lvgl_Init();

  Lvgl_Example1();
  // lv_demo_widgets();
  // lv_demo_benchmark();
  // lv_demo_keypad_encoder();
  // lv_demo_music();
  // lv_demo_printer();
  // lv_demo_stress();
  Camera_Init();
  xTaskCreatePinnedToCore(
    Driver_Loop,           
    "DriverTask",         
    4096,                 
    NULL,                 
    3,                    
    NULL,                 
    0                     
  );  

}
int Time_Loop=0;
void loop() {
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));

}
