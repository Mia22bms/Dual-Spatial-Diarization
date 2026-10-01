#include "Touch_CST328.h"

#if defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)

struct Touch_Data touch_data = {0};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2C
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Touch_I2C_Read(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write((uint8_t)(Reg_addr >> 8)); 
  Wire.write((uint8_t)Reg_addr);        
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - Touch I2C Read\r\n");
    return -1;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return 0;
}
bool Touch_I2C_Write(uint8_t Driver_addr, uint16_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write((uint8_t)(Reg_addr >> 8)); 
  Wire.write((uint8_t)Reg_addr);        
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if(Wire.endTransmission(true)){
    printf("The I2C transmission fails. - Touch I2C Write\r\n");
    return -1;
  }
  return 0;
}
uint8_t Touch_Init(void) {

  CST328_Touch_Reset();
  uint16_t Verification = CST328_Read_cfg();
  if(!((Verification==0xCACA)?true:false))
  printf("Touch initialization failed!\r\n");

  return ((Verification==0xCACA)?true:false);
}
/* Reset controller */
uint8_t CST328_Touch_Reset(void)
{
  Set_EXIO(TCA9555_EXIO1,LOW);
  vTaskDelay(pdMS_TO_TICKS(10));
  Set_EXIO(TCA9555_EXIO1,HIGH);
  vTaskDelay(pdMS_TO_TICKS(50));
  return true;
}
uint16_t CST328_Read_cfg(void) {

  uint8_t buf[24];
  Touch_I2C_Write(CST328_ADDR, HYN_REG_MUT_DEBUG_INFO_MODE, buf, 0);
  Touch_I2C_Read(CST328_ADDR, HYN_REG_MUT_DEBUG_INFO_BOOT_TIME,buf, 4);
  printf("TouchPad_ID:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[0], buf[1], buf[2], buf[3]);
  Touch_I2C_Read(CST328_ADDR, HYN_REG_MUT_DEBUG_INFO_BOOT_TIME, buf, 4);
  printf("TouchPad_X_MAX:%d    TouchPad_Y_MAX:%d \r\n", buf[1]*256+buf[0],buf[3]*256+buf[2]);

  Touch_I2C_Read(CST328_ADDR, HYN_REG_MUT_DEBUG_INFO_TP_NTX, buf, 24);
  printf("D1F4:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[0], buf[1], buf[2], buf[3]);
  printf("D1F8:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[4], buf[5], buf[6], buf[7]);
  printf("D1FC:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[8], buf[9], buf[10], buf[11]);
  printf("D200:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[12], buf[13], buf[14], buf[15]);
  printf("D204:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[16], buf[17], buf[18], buf[19]);
  printf("D208:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[20], buf[21], buf[22], buf[23]);
  printf("CACA Read:0x%04x\r\n", (((uint16_t)buf[11] << 8) | buf[10]));

  Touch_I2C_Write(CST328_ADDR, HYN_REG_MUT_NORMAL_MODE, buf, 0);
  return (((uint16_t)buf[11] << 8) | buf[10]);
}

// reads sensor and touches
// updates Touch Points, but if not touched, resets all Touch Point Information
uint8_t Touch_Read_Data(void) {
  uint8_t buf[41];
  uint8_t touch_cnt = 0;
  uint8_t clear = 0;
  uint8_t Over = 0xAB;
  size_t i = 0,num=0;
  Touch_I2C_Read(CST328_ADDR, ESP_LCD_TOUCH_CST328_READ_Number_REG, buf, 1);
  if ((buf[0] & 0x0F) == 0x00) {                                              
    Touch_I2C_Write(CST328_ADDR, ESP_LCD_TOUCH_CST328_READ_Number_REG, &clear, 1);  // No touch data
  } else {
    /* Count of touched points */
    touch_cnt = buf[0] & 0x0F;
    if (touch_cnt > TOUCH_MAX_POINTS || touch_cnt == 0) {
      Touch_I2C_Write(CST328_ADDR, ESP_LCD_TOUCH_CST328_READ_Number_REG, &clear, 1);
      return true;
    }
    /* Read all points */
    Touch_I2C_Read(CST328_ADDR, ESP_LCD_TOUCH_CST328_READ_XY_REG, &buf[1], 27);
    /* Clear all */
    Touch_I2C_Write(CST328_ADDR, ESP_LCD_TOUCH_CST328_READ_Number_REG, &clear, 1);
    // printf(" points=%d \r\n",touch_cnt);
    noInterrupts(); 
    /* Number of touched points */
    if(touch_cnt > TOUCH_MAX_POINTS)
        touch_cnt = TOUCH_MAX_POINTS;
    touch_data.points = (uint8_t)touch_cnt;
    /* Fill all coordinates */
    for (i = 0; i < touch_cnt; i++) {
      if(i>0) num = 2;
      touch_data.coords[i].x = (uint16_t)(((uint16_t)buf[(i * 5) + 2 + num] << 4) + ((buf[(i * 5) + 4 + num] & 0xF0)>> 4));               
      touch_data.coords[i].y = (uint16_t)(((uint16_t)buf[(i * 5) + 3 + num] << 4) + ( buf[(i * 5) + 4 + num] & 0x0F));
      touch_data.coords[i].strength = ((uint16_t)buf[(i * 5) + 5 + num]);
    }
    interrupts(); 
    // printf(" points=%d \r\n",touch_data.points);
  }
  return true;
}

#endif