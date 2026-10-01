#include "lvgl_example.h"

#include "lcd_driver.h"
#include "lvgl_driver.h"
#include "esp_log.h"

#include "bsp_board.h"
#include "camera_driver.h"
#include "esp_flash.h"   


static const char *TAG = "lvgl example";

typedef enum {
    MSG_TYPE_CAM_NONE, // 开启摄像头刷新
    MSG_TYPE_CAM_RUN_START, // 开启摄像头刷新
    MSG_TYPE_CAM_RUNNING, // 开启摄像头刷新
    MSG_TYPE_CAM_RUN_STOP,  // 停止摄像头刷新
} CamMessageType;

static lv_obj_t * SD_Size;
static lv_obj_t * FlashSize;
static lv_obj_t * Backlight_slider;
static lv_obj_t * img_camera;
static lv_image_dsc_t img_pic;
static lv_timer_t  * get_bat_vol_timer;

static QueueHandle_t xMessageQueue;


static void Onboard_create(lv_obj_t * parent);

static void swap_16bit_bytes(uint8_t *data, size_t length) {
    // 确保数据长度是2的倍数（16位数据）
    if (length % 2 != 0) {
        ESP_LOGE("camera", "Data length must be even for 16-bit swap");
        return;
    }
    
    // 遍历所有数据，交换每个16位数据的高低字节
    for (size_t i = 0; i < length; i += 2) {
        uint8_t temp = data[i];
        data[i] = data[i + 1];
        data[i + 1] = temp;
    }
}



void lv_example_1(void)
{
    xMessageQueue = xQueueCreate(5, sizeof(CamMessageType));

    lv_obj_t * tabview;
    tabview = lv_tabview_create(lv_screen_active());
    lv_obj_set_scroll_dir(lv_screen_active(), LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(lv_screen_active(), LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Onboard");
    lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "music");
    lv_obj_set_scroll_dir(tab2, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(tab2, LV_SCROLLBAR_MODE_OFF);

    Onboard_create(tab1);
    lvgl_music_create(tab2);

    lv_tabview_set_tab_bar_size(tabview,40);

}


void lvgl_set_backlight_slider_value(uint16_t backlight)
{
    lvgl_port_lock(0);
    lv_slider_set_value(Backlight_slider, backlight, LV_ANIM_ON); 
    Set_Backlight(backlight);
    lvgl_port_unlock();
    
}

static void Backlight_adjustment_event_cb(lv_event_t * e) 
{
  uint8_t Backlight = lv_slider_get_value(lv_event_get_target(e));  
  if (Backlight <= Backlight_MAX)  
  {
    lv_slider_set_value(Backlight_slider, Backlight, LV_ANIM_ON); 
    Set_Backlight(Backlight);
  }
  else
    printf("Backlight out of range: %d\n", Backlight);
}

uint32_t Flash_Searching(void)
{
    uint32_t Flash_Size;
    if(esp_flash_get_physical_size(NULL, &Flash_Size) == ESP_OK)
    {
        Flash_Size = Flash_Size / (uint32_t)(1024 * 1024);
        printf("Flash size: %ld MB\n", Flash_Size);
        return Flash_Size;
    }
    else{
        printf("Get flash size failed\n");
        return 0;
    }
    return 0;
}

static void sent_cam_task_queue_cmd(CamMessageType xMessage)
{
    if(xQueueSend(xMessageQueue, &xMessage, 0) != pdPASS)
    {
        ESP_LOGE(TAG,"sent queue err!!!");
    }
}

void vtask_camera_pic(void *pvParameters) 
{
    CamMessageType xReceivedMessage = MSG_TYPE_CAM_NONE;
    TickType_t xTicksToDelay = portMAX_DELAY;
    while (1) 
    {
        xQueueReceive(xMessageQueue, &xReceivedMessage, xTicksToDelay);
        
        switch (xReceivedMessage) 
        {
            case MSG_TYPE_CAM_NONE:

                break;
            case MSG_TYPE_CAM_RUN_START:    
                xTicksToDelay = pdMS_TO_TICKS(10);
                xReceivedMessage = MSG_TYPE_CAM_RUNNING;

                lvgl_port_lock(0);
                lv_obj_remove_flag(img_camera,LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(img_camera);
                lvgl_port_unlock();
                ESP_LOGI(TAG,"Camera start");

                break;
            case MSG_TYPE_CAM_RUNNING:
                camera_fb_t* fb = esp_camera_fb_get();
                if (!fb) {
                    ESP_LOGE(TAG,"Camera capture failed\n");
                    break;;
                }
                swap_16bit_bytes(fb->buf, fb->len);
                lvgl_port_lock(0);
                img_pic.data = fb->buf;
                lv_img_set_src(img_camera, &img_pic);
                lvgl_port_unlock();

                esp_camera_fb_return(fb);

                break;
            case MSG_TYPE_CAM_RUN_STOP: 
                xTicksToDelay = portMAX_DELAY;
                xReceivedMessage = MSG_TYPE_CAM_NONE;
                break;
            default:break;
        }

    }
}

static void btn_cam_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_remove_flag(img_camera,LV_OBJ_FLAG_HIDDEN);
        sent_cam_task_queue_cmd(MSG_TYPE_CAM_RUN_START);
    }

}

static void btn_cam_close_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_add_flag(img_camera,LV_OBJ_FLAG_HIDDEN);
        sent_cam_task_queue_cmd(MSG_TYPE_CAM_RUN_STOP);
        printf("关闭\r\n");
    }

}

static void Onboard_create(lv_obj_t * parent)
{
    /*Create a panel*/
    lv_obj_t * panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);
    lv_obj_t * panel1_title = lv_label_create(panel1);
    lv_label_set_text(panel1_title, "Onboard parameter");

    lv_obj_t * SD_label = lv_label_create(panel1);
    lv_label_set_text(SD_label, "SD Card");
    SD_Size = lv_textarea_create(panel1);
    lv_textarea_set_one_line(SD_Size, true);
    lv_textarea_set_placeholder_text(SD_Size, "SD Size");
    uint32_t sd_size = Get_SD_Size();
    if(sd_size == 0)
    {
        lv_textarea_set_placeholder_text(SD_Size, "No SD card is mounted");
    }
    else
    {
        char buf[100]; 
        snprintf(buf, sizeof(buf), "%ld MB\r\n", sd_size);
        lv_textarea_set_placeholder_text(SD_Size, buf);
    }
    
    //FLASH SIZE
    lv_obj_t * Flash_label = lv_label_create(panel1);
    lv_label_set_text(Flash_label, "Flash Size");
    FlashSize = lv_textarea_create(panel1);
    lv_textarea_set_one_line(FlashSize, true);
    lv_textarea_set_placeholder_text(FlashSize, "Flash Size");
    uint32_t flash_size = Flash_Searching();
    if(flash_size == 0)
    {
        lv_textarea_set_placeholder_text(FlashSize, "get flash size err");
    }
    else
    {
        char buf[100]; 
        snprintf(buf, sizeof(buf), "%ld MB\r\n", flash_size);
        lv_textarea_set_placeholder_text(FlashSize, buf);
    }

    lv_obj_t * camera_label = lv_label_create(panel1);
    lv_label_set_text(camera_label, "camera test");
    lv_obj_t * btn_cam_test= lv_button_create(panel1);
    lv_obj_add_event_cb(btn_cam_test, btn_cam_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_remove_flag(btn_cam_test, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_t * btn_label = lv_label_create(btn_cam_test);
    lv_label_set_text(btn_label, "Camera test");


    lv_obj_t * Backlight_label = lv_label_create(panel1);
    lv_label_set_text(Backlight_label, "Backlight brightness");
    Backlight_slider = lv_slider_create(panel1);                                 
    lv_obj_add_flag(Backlight_slider, LV_OBJ_FLAG_CLICKABLE);    
    lv_obj_set_size(Backlight_slider, 100, 20);              
    lv_obj_set_style_radius(Backlight_slider, 3, LV_PART_KNOB);               // Adjust the value for more or less rounding                                            
    lv_obj_set_style_bg_opa(Backlight_slider, LV_OPA_TRANSP, LV_PART_KNOB);                               
    // lv_obj_set_style_pad_all(Backlight_slider, 0, LV_PART_KNOB);                                            
    lv_obj_set_style_bg_color(Backlight_slider, lv_color_hex(0xAAAAAA), LV_PART_KNOB);               
    lv_obj_set_style_bg_color(Backlight_slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);             
    lv_obj_set_style_outline_width(Backlight_slider, 2, LV_PART_INDICATOR);  
    lv_obj_set_style_outline_color(Backlight_slider, lv_color_hex(0xD3D3D3), LV_PART_INDICATOR);      
    lv_slider_set_range(Backlight_slider, 5, Backlight_MAX);              
    lv_slider_set_value(Backlight_slider, DEFAULT_BACKLIGHT, LV_ANIM_ON);  
    lv_obj_add_event_cb(Backlight_slider, Backlight_adjustment_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    img_camera = lv_image_create(lv_screen_active());
    lv_obj_align(img_camera, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_foreground(img_camera);
    lv_obj_add_flag(img_camera,LV_OBJ_FLAG_HIDDEN);
    xTaskCreatePinnedToCore(vtask_camera_pic, "vtask_camera_pic task",4096, NULL, 3, NULL, 0);
    lv_obj_t * btn_cam_close= lv_button_create(img_camera);
    lv_obj_align(btn_cam_close, LV_ALIGN_TOP_RIGHT, 0, 10);
    lv_obj_set_size(btn_cam_close,50,50);
    lv_obj_add_event_cb(btn_cam_close, btn_cam_close_event_handler, LV_EVENT_CLICKED, NULL);

    img_pic.header.cf = LV_COLOR_FORMAT_RGB565,
    img_pic.header.magic = LV_IMAGE_HEADER_MAGIC,
    img_pic.header.w = 320,
    img_pic.header.h = 240,
    img_pic.data = NULL;
    img_pic.data_size = 320*240*2;

    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

    /*Create the top panel*/
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, //Avatar
                                        LV_GRID_CONTENT, //Name
                                        LV_GRID_CONTENT, //Description
                                        LV_GRID_CONTENT, //Email
                                        LV_GRID_CONTENT, //Phone number
                                        LV_GRID_CONTENT, //Button1
                                        LV_GRID_CONTENT, //Button2
                                        LV_GRID_TEMPLATE_LAST
                                        };

    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);


    /*Create the top panel*/
    static lv_coord_t grid_2_col_dsc[] = {LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(50), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_2_row_dsc[] = {
    LV_GRID_CONTENT,  /*Title*/
    5,                /*Separator*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_CONTENT,  /*Box title*/
    40,               /*Box*/
    LV_GRID_TEMPLATE_LAST               
    };

    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);
    lv_obj_set_grid_dsc_array(panel1, grid_2_col_dsc, grid_2_row_dsc);
    lv_obj_set_grid_cell(panel1_title, LV_GRID_ALIGN_START, 0, 5, LV_GRID_ALIGN_CENTER, 0, 1);


    lv_obj_set_grid_cell(SD_label, LV_GRID_ALIGN_START, 0, 5, LV_GRID_ALIGN_START, 2, 1);
    lv_obj_set_grid_cell(SD_Size, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_CENTER, 3, 1);

    lv_obj_set_grid_cell(Flash_label, LV_GRID_ALIGN_START, 0, 5, LV_GRID_ALIGN_START, 4, 1);
    lv_obj_set_grid_cell(FlashSize, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_CENTER, 5, 1);

    lv_obj_set_grid_cell(camera_label, LV_GRID_ALIGN_START, 0, 5, LV_GRID_ALIGN_START, 6, 1);
    lv_obj_set_grid_cell(btn_cam_test, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_CENTER, 7, 1);

    lv_obj_set_grid_cell(Backlight_label, LV_GRID_ALIGN_START, 0, 5, LV_GRID_ALIGN_START, 8, 1);
    lv_obj_set_grid_cell(Backlight_slider, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_CENTER, 9, 1);

}
















