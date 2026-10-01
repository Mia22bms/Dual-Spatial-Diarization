#include "touch_driver.h"
#include "esp_log.h"
#include "bsp_board.h"
#include "tca9555_driver.h"
#include "sdkconfig.h"

static const char *TAG = "touch driver";

esp_lcd_touch_handle_t tp_handle = NULL;

void touch_reset(void)
{
    Set_EXIO(IO_EXPANDER_PIN_NUM_1,0);
    vTaskDelay(pdMS_TO_TICKS(50));
    Set_EXIO(IO_EXPANDER_PIN_NUM_1,1);
    vTaskDelay(pdMS_TO_TICKS(50));
}



void touch_driver_init(void)
{
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    touch_reset();

    
    i2c_master_bus_handle_t i2c_handle = esp_ret_i2c_handle();
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_TOUCH_GPIO_RST,
        .int_gpio_num = EXAMPLE_TOUCH_GPIO_INT,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
        tp_cfg.x_max = EXAMPLE_LCD_H_RES,
        tp_cfg.y_max = EXAMPLE_LCD_V_RES,
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 0;
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE)
        tp_cfg.flags.swap_xy = 0;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 1;
    #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
        tp_cfg.x_max = EXAMPLE_LCD_H_RES,
        tp_cfg.y_max = EXAMPLE_LCD_V_RES,
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 0;
        tp_cfg.flags.mirror_y = 1;
    #endif


    #ifdef CONFIG_WAVESHARE_1_47INCH_TOUCH_LCD
        i2c_master_dev_handle_t dev_handle = NULL;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = ESP_LCD_TOUCH_IO_I2C_AXS5106_ADDRESS,
            .scl_speed_hz = 400000,
        };
        ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_handle, &dev_cfg, &dev_handle));


        esp_lcd_touch_config_t cfg = {
            .x_max = EXAMPLE_LCD_H_RES,
            .y_max = EXAMPLE_LCD_V_RES,
            .rst_gpio_num = -1,
            .int_gpio_num = EXAMPLE_TOUCH_GPIO_INT,
            .flags = {
                .swap_xy = 0,
                .mirror_x = 1,
                .mirror_y = 0,
            },
        };
        
            #ifdef CONFIG_EXAMPLE_DISPLAY_ROTATION_90_DEGREE
                cfg.x_max = EXAMPLE_LCD_V_RES,
                cfg.y_max = EXAMPLE_LCD_H_RES,
                cfg.flags.swap_xy = 1;
                cfg.flags.mirror_x = 0;
                cfg.flags.mirror_y = 0;
            #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_180_DEGREE)
                cfg.flags.swap_xy = 0;
                cfg.flags.mirror_x = 0;
                cfg.flags.mirror_y = 1;
            #elif defined(CONFIG_EXAMPLE_DISPLAY_ROTATION_270_DEGREE)
                cfg.x_max = EXAMPLE_LCD_V_RES,
                cfg.y_max = EXAMPLE_LCD_H_RES,
                cfg.flags.swap_xy = 1;
                cfg.flags.mirror_x = 1;
                cfg.flags.mirror_y = 1;
            #endif
        /* Initialize touch */
        ESP_LOGI("Touch", "Initialize touch controller AXS5106");
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_axs5106(dev_handle, &cfg, &tp_handle));
    #elif defined(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816_CONFIG();
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_handle, &tp_io_config, &tp_io_handle));
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp_handle));
    #elif defined(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST328_CONFIG();
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_handle, &tp_io_config, &tp_io_handle));
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst328(tp_io_handle, &tp_cfg, &tp_handle));
    #elif defined(CONFIG_WAVESHARE_3_5INCH_TOUCH_LCD)
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT6336_CONFIG();
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_handle, &tp_io_config, &tp_io_handle));
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft6336(tp_io_handle, &tp_cfg, &tp_handle));
    #endif


}