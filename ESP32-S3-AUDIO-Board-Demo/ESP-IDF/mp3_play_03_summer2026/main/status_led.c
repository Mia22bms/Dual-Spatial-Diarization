#include "status_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"

#include "bsp_board.h"   /* 复用 LED_STRIP_GPIO_PIN(38) / LED_STRIP_LED_COUNT(7) */

static const char *TAG = "status_led";

static led_strip_handle_t s_strip = NULL;

/* 把纯色按全局亮度上限缩放，避免电池供电下太亮/太耗电。 */
static inline uint8_t scale(uint8_t v)
{
    return (uint8_t)((uint16_t)v * STATUS_LED_BRIGHTNESS / 255);
}

esp_err_t status_led_init(void)
{
    if (s_strip != NULL) {
        return ESP_OK;   /* 幂等：已初始化 */
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num   = LED_STRIP_GPIO_PIN,
        .max_leds         = LED_STRIP_LED_COUNT,   /* 驱动整条 7 颗，但平时只点 index 0 */
        .led_model        = LED_MODEL_WS2812,
        /* 这块板的灯珠实际是 RGB 序：用 GRB 会把红/绿对调（蓝不变），
           表现为“绿心跳显示成红”。实测自检 红→绿→蓝 正常即确认此设置。 */
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src       = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,   /* 10MHz，WS2812 时序够用 */
        .mem_block_symbols = 0,              /* 用默认 */
        .flags = {
            .with_dma = false,               /* 单颗刷新极短，无需 DMA */
        },
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "led_strip 初始化失败: %s", esp_err_to_name(ret));
        s_strip = NULL;
        return ret;
    }

    led_strip_clear(s_strip);   /* 开机先全灭 */
    ESP_LOGI(TAG, "状态灯就绪：GPIO%d，共%d颗，亮度上限%d",
             LED_STRIP_GPIO_PIN, LED_STRIP_LED_COUNT, STATUS_LED_BRIGHTNESS);
    return ESP_OK;
}

esp_err_t status_led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 其余 6 颗强制灭，只点 index 0 —— clear 会把缓冲清零，再设 0 号即可。 */
    led_strip_clear(s_strip);
    led_strip_set_pixel(s_strip, 0, scale(r), scale(g), scale(b));
    return led_strip_refresh(s_strip);
}

esp_err_t status_led_off(void)
{
    if (s_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return led_strip_clear(s_strip);
}

void status_led_selftest_loop(void)
{
    ESP_LOGI(TAG, "自检开始：index 0 循环 红→绿→蓝，其余灯灭");
    while (1) {
        status_led_set_rgb(255, 0, 0);   /* 红 */
        vTaskDelay(pdMS_TO_TICKS(500));
        status_led_set_rgb(0, 255, 0);   /* 绿 */
        vTaskDelay(pdMS_TO_TICKS(500));
        status_led_set_rgb(0, 0, 255);   /* 蓝 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ── 录音固件四状态 ───────────────────────────────────────── */

void status_led_standby(void)
{
    status_led_set_rgb(0, 0, 255);   /* 蓝常亮 */
}

void status_led_error_halt(void)
{
    ESP_LOGE(TAG, "SD 报警态：红灯快闪，停住");
    while (1) {
        status_led_set_rgb(255, 0, 0);   /* 红 */
        vTaskDelay(pdMS_TO_TICKS(100));
        status_led_off();
        vTaskDelay(pdMS_TO_TICKS(100));  /* 100+100ms → 5Hz */
    }
}

/* 心跳内部状态：计数器 + 当前亮/灭相位。 */
static uint32_t s_hb_count = 0;
static bool     s_hb_on    = false;

void status_led_record_begin(void)
{
    s_hb_count = 0;
    s_hb_on    = true;
    status_led_set_rgb(0, 255, 0);   /* 绿灯先亮起 */
}

void status_led_record_tick(bool write_ok)
{
    if (!write_ok) {
        return;   /* 写入异常：不推进心跳，灯停在当前相位（循环马上会 break） */
    }
    if (++s_hb_count < STATUS_LED_HEARTBEAT_TICKS) {
        return;   /* 多数情况只自增就返回，不碰 RMT */
    }
    s_hb_count = 0;
    s_hb_on    = !s_hb_on;
    if (s_hb_on) {
        status_led_set_rgb(0, 255, 0);   /* 绿亮 */
    } else {
        status_led_off();
    }
}

void status_led_recording_closed(void)
{
    status_led_set_rgb(0, 255, 0);   /* 绿常亮 */
}
