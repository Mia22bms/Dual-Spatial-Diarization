/*****************************************************************************
  | File        :   LVGL_Driver.c
  
  | help        : 
    The provided LVGL library file must be installed first
******************************************************************************/
#include "LVGL_Driver.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[ LVGL_BUF_LEN ];
static lv_color_t buf2[ LVGL_BUF_LEN];
// static lv_color_t* buf1 = (lv_color_t*) heap_caps_malloc(LVGL_BUF_LEN , MALLOC_CAP_SPIRAM);
// static lv_color_t* buf2 = (lv_color_t*) heap_caps_malloc(LVGL_BUF_LEN , MALLOC_CAP_SPIRAM);
    
/*  Display flushing 
    Displays LVGL content on the LCD
    This function implements associating LVGL data to the LCD screen
*/
void Lvgl_Display_LCD( lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map )
{
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;
  // copy a buffer's content to a specific area of the display
#ifdef CONFIG_LVGL_COLOR_16_SWAP 
  if(!camera_Show_Flag){
    uint32_t size = (offsetx2 - offsetx1 +1 ) * (offsety2 - offsety1 + 1);
    uint16_t * color = (uint16_t *)color_map;
    for (size_t i = 0; i < size; i++) {
      color[i] = (((color[i] >> 8) & 0xFF) | ((color[i] << 8) & 0xFF00));
    }
  }
#else
  if(camera_Show_Flag){
    uint32_t size = (offsetx2 - offsetx1 +1 ) * (offsety2 - offsety1 + 1);
    uint16_t * color = (uint16_t *)color_map;
    for (size_t i = 0; i < size; i++) {
      color[i] = (((color[i] >> 8) & 0xFF) | ((color[i] << 8) & 0xFF00));
    }
  }
#endif
  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 +1, offsety2 + 1, color_map);
  lv_disp_flush_ready(drv);
}

/*Read the touchpad*/
void Lvgl_Touchpad_Read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
{
  #if defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
  int16_t x[1], y[1];
  uint8_t touched = touch.getPoint(x, y, 1);

  if (touched) {

    if(Touch_mirror_x){
    if(!Touch_swap_xy)
      x[0] = EXAMPLE_LCD_WIDTH - x[0];
    else
      x[0] = EXAMPLE_LCD_HEIGHT - x[0];
    }
    if(Touch_mirror_y){
      if(!Touch_swap_xy)
        y[0] = EXAMPLE_LCD_HEIGHT - y[0];
      else
        y[0] = EXAMPLE_LCD_WIDTH - y[0];
    }
    if(Touch_swap_xy){
      uint16_t swap_xy = x[0];
      x[0] = y[0];
      y[0] = swap_xy;
    }



    data->state = LV_INDEV_STATE_PR;

    //Set the coordinates
    data->point.x = x[0];
    data->point.y = y[0];
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
  #else
  Touch_Read_Data();
  if(Touch_mirror_x){
    if(!Touch_swap_xy)
      touch_data.coords[0].x = EXAMPLE_LCD_WIDTH - touch_data.coords[0].x;
    else
      touch_data.coords[0].x = EXAMPLE_LCD_HEIGHT - touch_data.coords[0].x;
  }
  if(Touch_mirror_y){
    if(!Touch_swap_xy)
      touch_data.coords[0].y = EXAMPLE_LCD_HEIGHT - touch_data.coords[0].y;
    else
      touch_data.coords[0].y = EXAMPLE_LCD_WIDTH - touch_data.coords[0].y;
  }
  if(Touch_swap_xy){
    uint16_t swap_xy = touch_data.coords[0].x;
    touch_data.coords[0].x = touch_data.coords[0].y;
    touch_data.coords[0].y = swap_xy;
  }
  if (touch_data.points != 0x00) {
    data->point.x = touch_data.coords[0].x;
    data->point.y = touch_data.coords[0].y;
    data->state = LV_INDEV_STATE_PR;
    printf("LVGL : X=%u Y=%u points=%d\r\n",  touch_data.coords[0].x , touch_data.coords[0].y,touch_data.points);
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
  if (touch_data.gesture != NONE ) {    
  }
  touch_data.coords[0].x = 0;
  touch_data.coords[0].y = 0;
  touch_data.points = 0;
  touch_data.gesture = NONE;
  #endif


}





void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}
void example_increase_lvgl_Loop_tick(void *arg)
{
  lv_timer_handler(); /* let the GUI do its work */
}
void Lvgl_Init(void)
{
  lv_init();
  lv_disp_draw_buf_init( &draw_buf, buf1, buf2, LVGL_BUF_LEN);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( &disp_drv );
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = Lvgl_Display_LCD;
  disp_drv.full_refresh = 1;                    /**< 1: Always make the whole screen redrawn*/
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register( &disp_drv );

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( &indev_drv );
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = Lvgl_Touchpad_Read;
  lv_indev_drv_register( &indev_drv );

  /* Create simple label */
  lv_obj_t *label = lv_label_create( lv_scr_act());
  lv_label_set_text( label, "Hello Ardino and LVGL!");
  lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

}
void Lvgl_Loop(void)
{
  lv_timer_handler(); /* let the GUI do its work */
}
void refresh_screen() {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_invalidate(screen);
}
