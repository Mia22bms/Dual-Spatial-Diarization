#include "camera_driver.h"
#include "esp_log.h"
#include "tca9555_driver.h"

static const char *TAG = "camera driver";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


void Camera_EN(){
    Set_EXIO(IO_EXPANDER_PIN_NUM_5,false);
    vTaskDelay(pdMS_TO_TICKS(50));
}
void Camera_DIS(){
    Set_EXIO(IO_EXPANDER_PIN_NUM_5,true);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void Camera_Set_GPIOA(){
    Set_EXIO(IO_EXPANDER_PIN_NUM_6,true);   // 使用 Tx 、Rx 用于摄像头引脚
    vTaskDelay(pdMS_TO_TICKS(50));
}
void Camera_Set_GPIOB(){
    Set_EXIO(IO_EXPANDER_PIN_NUM_6,false);   // 使用 USB 的 DN、DP 用于摄像头引脚
    vTaskDelay(pdMS_TO_TICKS(50));
}

esp_err_t Camera_Driver_Init(void)
{
    Camera_Set_GPIOA();
    Camera_EN();
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    return ESP_OK;
}

