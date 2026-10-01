#include "lcd_driver.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "bsp_board.h"
#include "tca9555_driver.h"
#include "sdkconfig.h"
#include "driver/ledc.h"

static const char *TAG = "lcd driver";

/* LCD IO and panel */
esp_lcd_panel_io_handle_t lcd_io = NULL;
esp_lcd_panel_handle_t lcd_panel = NULL;


static uint8_t backlight = 0;
static ledc_channel_config_t ledc_channel;

void lcd_reset(void)
{
    Set_EXIO(IO_EXPANDER_PIN_NUM_0,0);
    vTaskDelay(pdMS_TO_TICKS(10));
    Set_EXIO(IO_EXPANDER_PIN_NUM_0,1);
    vTaskDelay(pdMS_TO_TICKS(50));
}


void Backlight_Init(void)
{
    ESP_LOGI("LCD_Driver", "Turn on LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel.channel    = LEDC_CHANNEL_0;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = EXAMPLE_LCD_GPIO_BL;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.timer_sel  = LEDC_TIMER_0;
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);
    
    Set_Backlight(DEFAULT_BACKLIGHT);      //0~100    
}

static void test_draw_bitmap(esp_lcd_panel_handle_t panel_handle)
{
    uint16_t row_line = ((320 / 16) << 1) >> 1;
    uint8_t byte_per_pixel = 16 / 8;
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * 240 * byte_per_pixel, MALLOC_CAP_DMA);

    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < row_line * 240; i++) {
            for (int k = 0; k < byte_per_pixel; k++) {
                color[i * byte_per_pixel + k] = (SPI_SWAP_DATA_TX(BIT(j), 16) >> (k * 8)) & 0xff;
            }
        }
        esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, 240, (j + 1) * row_line, color);
    }
    free(color);

    vTaskDelay(pdMS_TO_TICKS(1000));
}

esp_err_t lcd_driver_init(void)
{
    lcd_reset();
    Backlight_Init();
    esp_err_t ret = ESP_OK;

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SCLK,
        .mosi_io_num = EXAMPLE_LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
        #if defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE) || defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
        .max_transfer_sz = EXAMPLE_LCD_V_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
        #endif
        
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_GPIO_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
        
    };

    #ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
    panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_jd9853(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");
    #elif defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
    panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");
    #elif defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
    panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");
    #elif defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7796(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");
    #endif


    #ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
        #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel, 0, 34));

        #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE)
            ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel, 34, 0));
        #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
            ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel, 0, 34));
        #else
            ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel, 34, 0));
        #endif
    #endif

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_invert_color(lcd_panel, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    Set_Backlight(DEFAULT_BACKLIGHT);
    
    //test_draw_bitmap(lcd_panel);
    return ret;

err:
    if (lcd_panel) {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io) {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}



void Set_Backlight(uint8_t Light)
{   
    if(Light > Backlight_MAX) Light = Backlight_MAX;
    backlight = Light;
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, backlight);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

    if(Light > Backlight_MAX) Light = Backlight_MAX;
    uint16_t Duty = LEDC_MAX_Duty-(81*(Backlight_MAX-Light));
    if(Light == 0) 
        Duty = 0;
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, Duty);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

uint8_t Read_Backlight_value(void)
{
    return backlight;
}