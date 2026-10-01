#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif



#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 43
#define CAM_PIN_SIOD -1
#define CAM_PIN_SIOC -1

#define CAM_PIN_D7 48
#define CAM_PIN_D6 47
#define CAM_PIN_D5 46
#define CAM_PIN_D4 45
#define CAM_PIN_D3 39
#define CAM_PIN_D2 18
#define CAM_PIN_D1 17
#define CAM_PIN_D0 2
#define CAM_PIN_VSYNC 21
#define CAM_PIN_HREF 1
#define CAM_PIN_PCLK 44

esp_err_t Camera_Driver_Init(void);

#ifdef __cplusplus
}
#endif

