#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "bsp_board.h"
#include "tca9555_driver.h"
#include "lcd_driver.h"
#include "touch_driver.h"
#include "lvgl_driver.h"
#include "mic_speech.h"
#include "audio_driver.h"
#include "demos/lv_demos.h"
#include "lvgl_example.h"
#include "camera_driver.h"

static char *TAG = "app main";



void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(16000, 2, 16));
    esp_sdcard_init("/sdcard", 10);
    tca9555_driver_init();
    Speech_Init();
    Audio_Play_Init();

    LVGL_Search_Music();

    lcd_driver_init();
    touch_driver_init();
    lvgl_driver_init();
    Camera_Driver_Init();

    lvgl_port_lock(0);
    lv_example_1();
    lvgl_port_unlock();

}
