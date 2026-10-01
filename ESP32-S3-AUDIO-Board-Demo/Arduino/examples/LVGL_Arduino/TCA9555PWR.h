#pragma once

#include <stdio.h>
#include "I2C_Driver.h"

#define TCA9555_EXIO0   0
#define TCA9555_EXIO1   1
#define TCA9555_EXIO2   2
#define TCA9555_EXIO3   3
#define TCA9555_EXIO4   4
#define TCA9555_EXIO5   5
#define TCA9555_EXIO6   6
#define TCA9555_EXIO7   7
#define TCA9555_EXIO8   8
#define TCA9555_EXIO9   9
#define TCA9555_EXIO10  10
#define TCA9555_EXIO11  11
#define TCA9555_EXIO12  12
#define TCA9555_EXIO13  13
#define TCA9555_EXIO14  14
#define TCA9555_EXIO15  15

/****************************************************** The macro defines the TCA9555PWR information ******************************************************/ 
#define TCA9555_ADDRESS         0x20                      // TCA9555PWR I2C address

/********************************************************** Set EXIO mode **********************************************************/       
// State  = INPUT, OUTPUT
void Mode_EXIO(uint8_t Pin,uint8_t State);                  // Set the mode of the TCA9555PWR Pin. The default is Output mode (output mode or input mode). State: 0= Output mode 1= input mode   
void Mode_EXIOS(uint16_t PinState);                         // Set the mode of the 7 pins from the TCA9555PWR with PinState  
/********************************************************** Read EXIO status **********************************************************/       
uint8_t Read_EXIO(uint8_t Pin);                             // Read the level of the TCA9555PWR Pin
uint16_t Read_EXIOS(void);                                  // Read the level of all pins of TCA9555PWR, the default read input level state, want to get the current IO output state, pass the parameter TCA9555_OUTPUT_REG, such as Read_EXIOS(TCA9555_OUTPUT_REG);
/********************************************************** Set the EXIO output status **********************************************************/  
void Set_EXIO(uint8_t Pin,uint8_t State);                   // Sets the level state of the Pin without affecting the other pins
void Set_EXIOS(uint16_t PinState);                          // Set 7 pins to the PinState state such as :PinState=0x23, 0010 0011 state (the highest bit is not used)
/********************************************************** Flip EXIO state **********************************************************/  
void Set_Toggle(uint8_t Pin);                               // Flip the level of the TCA9555PWR Pin
/********************************************************* TCA9555PWR Initializes the device ***********************************************************/  
void TCA9555PWR_Init(uint16_t PinState);                    // Set the seven pins to PinState state, for example :PinState=0x23, 0010 0011 State (the highest bit is not used) (Output mode or input mode) 0= Output mode 1= Input mode. The default value is output mode
