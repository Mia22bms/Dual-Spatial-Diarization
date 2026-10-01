#pragma once
#include "Arduino.h"
#include "TCA9555PWR.h"

#define BOOT_KEY_PIN          0

#define BUTTON_ACTIVE_LEVEL   0
#define Button_PIN1           BOOT_KEY_PIN

extern uint8_t BOOT_State;    
extern uint8_t KEY1_State;   
extern uint8_t KEY2_State;   
extern uint8_t KEY3_State;   

void Button_Init(void);
