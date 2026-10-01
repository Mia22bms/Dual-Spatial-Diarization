#include "TCA9555PWR.h"
#include "TCA9555.h"

// TCA9555 TCA(TCA9555_ADDRESS, &Wire);
TCA9555 TCA(TCA9555_ADDRESS);
/********************************************************** Set EXIO mode **********************************************************/       
void Mode_EXIO(uint8_t Pin,uint8_t State)                 // Set the mode of the TCA9555PWR Pin. The default is Output mode (output mode or input mode). State: 0= Output mode 1= input mode   
{
  bool result = TCA.pinMode1(Pin,State); 
  if (!result) { 
    printf("I/O Configuration Failure !!!\r\n");
  }
}
void Mode_EXIOS(uint16_t PinState)                         // Set the mode of the 7 pins from the TCA9555PWR with PinState   
{
  bool result = TCA.pinMode16(PinState);  
  if (!result) {   
    printf("I/O Configuration Failure !!!\r\n");
  }
}
/********************************************************** Read EXIO status **********************************************************/       
uint8_t Read_EXIO(uint8_t Pin)                            // Read the level of the TCA9555PWR Pin
{
  uint8_t bitStatus = TCA.read1(Pin);          
  return bitStatus;                                  
}
uint16_t Read_EXIOS(void)       // Read the level of all pins of TCA9555PWR, the default read input level state, want to get the current IO output state, pass the parameter TCA9555_OUTPUT_REG, such as Read_EXIOS(TCA9555_OUTPUT_REG);
{
  uint16_t inputBits = TCA.read16();                     
  return inputBits;     
}

/********************************************************** Set the EXIO output status **********************************************************/  
void Set_EXIO(uint8_t Pin,uint8_t State)                  // Sets the level state of the Pin without affecting the other pins
{
  bool result = TCA.write1(Pin,State);  
  if (!result) {                         
    printf("Failed to set EXIO!!!\r\n");
  }
}
void Set_EXIOS(uint16_t PinState)                          // Set 7 pins to the PinState state such as :PinState=0x23, 0010 0011 state (the highest bit is not used)
{
  bool result = TCA.write16(PinState); 
  if (!result) {                  
    printf("Failed to set EXIO!!!\r\n");
  }
}
/********************************************************** Flip EXIO state **********************************************************/  
void Set_Toggle(uint8_t Pin)                              // Flip the level of the TCA9555PWR Pin
{
    uint8_t bitsStatus = Read_EXIO(Pin);                 
    Set_EXIO(Pin,!bitsStatus); 
}
/********************************************************* TCA9555PWR Initializes the device ***********************************************************/  
void TCA9555PWR_Init(uint16_t PinState)                  // Set the seven pins to PinState state, for example :PinState=0x23, 0010 0011 State  (Output mode or input mode) 0= Output mode 1= Input mode. The default value is output mode
{      
  TCA.begin();            
  Mode_EXIOS(PinState); 
}
