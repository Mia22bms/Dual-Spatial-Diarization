#include "Touch_AXS5106.h"

#if defined(CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD)

struct Touch_Data touch_data = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2C
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool I2C_Read_Touch(uint16_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr); 
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Read\r\n");
    return true;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return true;
}
bool I2C_Write_Touch(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);       
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if(Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Write\r\n");
    return false;
  }
  return true;
}
uint8_t Touch_Init(void) {
  AXS5106_Touch_Reset();
  return true;
}
/* Reset controller */
uint8_t AXS5106_Touch_Reset(void)
{
  Set_EXIO(TCA9555_EXIO1,LOW);
  vTaskDelay(pdMS_TO_TICKS(10));
  Set_EXIO(TCA9555_EXIO1,HIGH);
  vTaskDelay(pdMS_TO_TICKS(50));
  return true;
}
// reads sensor and touchesd
uint8_t Touch_Read_Data(void) {
  uint8_t buf[30];
  uint8_t touchpad_cnt = 0;
  I2C_Read_Touch(AXS5106_ADDR,TOUCH_AXS5106_TOUCH_POINTS_REG, buf, 14);
  
  if (buf[1] != 0x00) {        
    noInterrupts(); 
    /* Number of touched points */
    touch_data.points = buf[1] & 0x0F;
    if(touch_data.points > TOUCH_MAX_POINTS)
      touch_data.points = TOUCH_MAX_POINTS;
    /* Fill coordinates */
    for (int i = 0; i < touch_data.points; i++)
    {
      touch_data.coords[i].y = (((uint16_t)(buf[4 + i * 6] & 0x0f)) << 8);
      touch_data.coords[i].y |= buf[5 + i * 6];
      touch_data.coords[i].x = ((uint16_t)(buf[2 + i * 6] & 0x0f)) << 8;
      touch_data.coords[i].x |= buf[3 + i * 6];
    }
    interrupts(); 
  }
  return true;
}
#endif