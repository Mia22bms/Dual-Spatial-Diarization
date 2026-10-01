#include "Button_Driver.h"

uint8_t BOOT_State = 0;
uint8_t KEY1_State = 0;
uint8_t KEY2_State = 0;
uint8_t KEY3_State = 0;      

void  ESP32_Button_init(void){
  pinMode(Button_PIN1, INPUT); 
}

void Timer_Callback(void *param)
{
  while(1)
  {         
    if(Read_EXIO(TCA9555_EXIO9) == BUTTON_ACTIVE_LEVEL){
      vTaskDelay(pdMS_TO_TICKS(10));
      if(Read_EXIO(TCA9555_EXIO9) == BUTTON_ACTIVE_LEVEL){
        KEY1_State = 1;
        while(Read_EXIO(TCA9555_EXIO9) == BUTTON_ACTIVE_LEVEL){
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }
    if(Read_EXIO(TCA9555_EXIO10) == BUTTON_ACTIVE_LEVEL){
      vTaskDelay(pdMS_TO_TICKS(10));
      if(Read_EXIO(TCA9555_EXIO10) == BUTTON_ACTIVE_LEVEL){
        KEY2_State = 1; 
        while(Read_EXIO(TCA9555_EXIO10) == BUTTON_ACTIVE_LEVEL){
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }
    if(Read_EXIO(TCA9555_EXIO11) == BUTTON_ACTIVE_LEVEL){
      vTaskDelay(pdMS_TO_TICKS(10));
      if(Read_EXIO(TCA9555_EXIO11) == BUTTON_ACTIVE_LEVEL){
        KEY3_State = 1; 
        while(Read_EXIO(TCA9555_EXIO11) == BUTTON_ACTIVE_LEVEL){
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }
    if(digitalRead(Button_PIN1) == BUTTON_ACTIVE_LEVEL){
      vTaskDelay(pdMS_TO_TICKS(10));
      if(digitalRead(Button_PIN1) == BUTTON_ACTIVE_LEVEL){
        BOOT_State = 1; 
        while(digitalRead(Button_PIN1) == BUTTON_ACTIVE_LEVEL){
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  vTaskDelete(NULL);                                              
}
void Button_Init(void)
{
  ESP32_Button_init(); 
  xTaskCreatePinnedToCore(Timer_Callback, "Timer_task", 4096, NULL, 3, NULL,  0);      
}

