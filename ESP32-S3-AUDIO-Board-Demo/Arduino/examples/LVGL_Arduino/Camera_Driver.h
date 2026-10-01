#pragma once
#include "esp_camera.h"
#include "I2C_Driver.h"
#include "LVGL_Driver.h"
#include "LVGL_Example.h"
#include "Button_Driver.h"

#define PWDN_GPIO_NUM   -1  // power down is not used
#define RESET_GPIO_NUM  -1 // software reset will be performed
#define XCLK_GPIO_NUM   43
#define SIOD_GPIO_NUM   -1
#define SIOC_GPIO_NUM   -1

#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 47
#define Y7_GPIO_NUM 46
#define Y6_GPIO_NUM 45
#define Y5_GPIO_NUM 39
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 2
#define VSYNC_GPIO_NUM 21
#define HREF_GPIO_NUM 1
#define PCLK_GPIO_NUM 44

extern bool camera_Show_Flag;
void Camera_Init(void);
void camera_task(void *param);