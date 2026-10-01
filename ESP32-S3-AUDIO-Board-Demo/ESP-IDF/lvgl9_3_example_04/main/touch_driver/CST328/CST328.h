#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#define ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS (0x1A)
//workmode
#define CST328_REG_DEBUG_INFO_MODE              0xD101
#define CST328_REG_RESET_MODE            	    0xD102
#define CST328_REG_DEBUG_RECALIBRATION_MODE     0xD104
#define CST328_REG_DEEP_SLEEP_MODE    		    0xD105
#define CST328_REG_DEBUG_POINT_MODE	    	    0xD108
#define CST328_REG_NORMAL_MODE                  0xD109

#define CST328_REG_DEBUG_RAWDATA_MODE           0xD10A
#define CST328_REG_DEBUG_DIFF_MODE              0xD10D
#define CST328_REG_DEBUG_FACTORY_MODE           0xD119
#define CST328_REG_DEBUG_FACTORY_MODE_2         0xD120
//debug info
/****************CST328_REG_DEBUG_INFO_MODE address start***********/
#define CST328_REG_DEBUG_INFO_BOOT_TIME         0xD1FC
#define CST328_REG_DEBUG_INFO_RES_Y             0xD1FA
#define CST328_REG_DEBUG_INFO_RES_X             0xD1F8
#define CST328_REG_DEBUG_INFO_KEY_NUM           0xD1F7
#define CST328_REG_DEBUG_INFO_TP_NRX            0xD1F6
#define CST328_REG_DEBUG_INFO_TP_NTX            0xD1F4

/* CST328 registers */
#define ESP_LCD_TOUCH_CST328_READ_Number_REG    (0xD005)
#define ESP_LCD_TOUCH_CST328_READ_XY_REG        (0xD000)
#define ESP_LCD_TOUCH_CST328_READ_Checksum_REG  (0x80FF)
#define ESP_LCD_TOUCH_CST328_CONFIG_REG         (0x8047)




/**
 * @brief Touch IO configuration structure
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_CST328_CONFIG()           \
    {                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS, \
        .scl_speed_hz = 400000,     /*!< Set the I2C master clock frequency */  \
        .control_phase_bytes = 1,           \
        .dc_bit_offset = 0,                 \
        .lcd_cmd_bits = 16,                 \
        .flags =                            \
        {                                   \
            .disable_control_phase = 1,     \
        }                                   \
    }


esp_err_t esp_lcd_touch_new_i2c_cst328(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch);


