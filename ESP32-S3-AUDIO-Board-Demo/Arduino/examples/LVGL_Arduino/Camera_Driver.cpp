#include "Camera_Driver.h"


lv_obj_t *img_camera;




void Camera_EN(){
  Set_EXIO(TCA9555_EXIO5,LOW);
  vTaskDelay(pdMS_TO_TICKS(50));
}
void Camera_DIS(){
    Set_EXIO(TCA9555_EXIO5,HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void Camera_Set_GPIOA(){
  Set_EXIO(TCA9555_EXIO6,HIGH);   // 使用 Tx 、Rx 用于摄像头引脚
  vTaskDelay(pdMS_TO_TICKS(50));
}
void Camera_Set_GPIOB(){
  Set_EXIO(TCA9555_EXIO6,LOW);   // 使用 USB 的 DN、DP 用于摄像头引脚
  vTaskDelay(pdMS_TO_TICKS(50));
}
void lvgl_camera_ui_init(lv_obj_t *parent) {
  // lv_obj_t *obj = lv_obj_create(parent);
  // lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
  img_camera = lv_img_create(parent);
  // lv_obj_set_size(img_camera, 240,);
  // lv_img_set_angle(img_camera, 900);
  lv_obj_align(img_camera, LV_ALIGN_CENTER, 0, 0);  // 居中显示
  lv_obj_set_pos(img_camera, 0, 0);
  lv_obj_set_scroll_dir(parent, LV_DIR_NONE);

  lv_obj_set_style_pad_top(img_camera, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(img_camera, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(img_camera, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(img_camera, 0, LV_PART_MAIN);
}

void Camera_Init(void){
  Camera_Set_GPIOA();
  Camera_EN();
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_1;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.sccb_i2c_port = I2C_MASTER_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  
    
#ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
    #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
        config.frame_size = FRAMESIZE_QVGA;
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
        config.frame_size = FRAMESIZE_QVGA;
    #else
        config.frame_size = FRAMESIZE_HVGA;
    #endif
#elif defined(CONFIG_WAVESHARE_1_85INCH_TOUCH_LCD)
    config.frame_size = FRAMESIZE_VGA;
#elif defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
    #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
        config.frame_size = FRAMESIZE_QVGA;
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
        config.frame_size = FRAMESIZE_QVGA;
    #else
      config.frame_size = FRAMESIZE_HVGA;
    #endif
#elif defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
    #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
        config.frame_size = FRAMESIZE_QVGA;
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
        config.frame_size = FRAMESIZE_QVGA;
    #else
      config.frame_size = FRAMESIZE_HVGA;
    #endif
#endif
  config.pixel_format = PIXFORMAT_RGB565;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    printf("Camera init failed with error 0x%x\r\n", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if(s == NULL){
    return;
  }
  s->set_hmirror(s, 1);
  xTaskCreatePinnedToCore(camera_task, "camera_task_task", 1024 * 3, NULL, 3, NULL, 0);
}

bool camera_Show_Flag = 0;
void camera_task(void *param) {
  lvgl_camera_ui_init(lv_scr_act());
  refresh_screen();
  bool Boot_Change = 1;
  camera_fb_t *pic;
  lv_img_dsc_t img_dsc;
  img_dsc.header.always_zero = 0;

  img_dsc.header.w = EXAMPLE_LCD_WIDTH;
  img_dsc.header.h = EXAMPLE_LCD_HEIGHT;

  img_dsc.data_size = EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * 2;
  img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  img_dsc.data = NULL;

  while (1) {
    
    if(img_camera == NULL){
      lvgl_camera_ui_init(lv_scr_act());
    }
    
    if(BOOT_State){ 
      camera_Show_Flag = !camera_Show_Flag;
      Boot_Change = 1;
      BOOT_State = 0; 
    }  
    if(Boot_Change){
      Boot_Change = 0;
      if(!camera_Show_Flag){
        ESP_LOGI("camera", "Display switching Basic example");
        lv_obj_add_flag(img_camera, LV_OBJ_FLAG_HIDDEN);
        lv_timer_resume(auto_step_timer);
        refresh_screen();
      }
      else{
        ESP_LOGI("camera", "Display switching camera");
        lv_obj_clear_flag(img_camera, LV_OBJ_FLAG_HIDDEN); 
        lv_timer_pause(auto_step_timer);
        refresh_screen();
      }
    }
    if(camera_Show_Flag){
      pic = esp_camera_fb_get();
      if (pic == NULL) {
        ESP_LOGW("camera", "Failed to get frame");
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }
      img_dsc.data = pic->buf;
      lv_img_set_src(img_camera, &img_dsc);
      esp_camera_fb_return(pic);
      vTaskDelay(pdMS_TO_TICKS(5));
    }
    else{
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
  vTaskDelete(NULL);
}
