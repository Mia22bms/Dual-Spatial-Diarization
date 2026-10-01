#pragma once
#include "Board_Configuration.h"

#if defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD)
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_intr_alloc.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "lvgl.h"

#include "esp_lcd_st77916.h"
#include "TCA9555PWR.h"


#ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
#define EXAMPLE_LCD_WIDTH                   (360)
#define EXAMPLE_LCD_HEIGHT                  (360)
#elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
#define EXAMPLE_LCD_WIDTH                   (360)
#define EXAMPLE_LCD_HEIGHT                  (360)
#else
#define EXAMPLE_LCD_WIDTH                   (360)
#define EXAMPLE_LCD_HEIGHT                  (360)
#endif
#define EXAMPLE_LCD_COLOR_BITS              (16)

#define ESP_PANEL_HOST_SPI_ID_DEFAULT       (SPI2_HOST)
#define ESP_PANEL_LCD_SPI_MODE              (0)                   // 0/1/2/3, typically set to 0
#define ESP_PANEL_LCD_SPI_CLK_HZ            (80 * 1000 * 1000)    // Should be an integer divisor of 80M, typically set to 40M
#define ESP_PANEL_LCD_SPI_TRANS_QUEUE_SZ    (10)                  // Typically set to 10
#define ESP_PANEL_LCD_SPI_CMD_BITS          (32)                  // Typically set to 32
#define ESP_PANEL_LCD_SPI_PARAM_BITS        (8)                   // Typically set to 8

#define ESP_PANEL_LCD_SPI_IO_TE             (-1)
#define ESP_PANEL_LCD_SPI_IO_SCK            (4)
#define ESP_PANEL_LCD_SPI_IO_DATA0          (9)
#define ESP_PANEL_LCD_SPI_IO_DATA1          (8)
#define ESP_PANEL_LCD_SPI_IO_DATA2          (7)
#define ESP_PANEL_LCD_SPI_IO_DATA3          (6)
#define ESP_PANEL_LCD_SPI_IO_CS             (3)
#define EXAMPLE_LCD_PIN_NUM_RST             (-1)    // EXIO2
#define EXAMPLE_LCD_PIN_NUM_BK_LIGHT        (5)

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL       (1)
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE   (2048)
  

extern esp_lcd_panel_handle_t panel_handle;
void ST77916_Init();

void LCD_Init(void);                     // Call this function to initialize the screen (must be called in the main function) !!!!!

#endif