#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 板载 WS2812 RGB 灯带的状态灯封装。
 *
 * 这一层是为之后的「录音状态灯」准备的可复用底座，不是一次性测试脚本：
 *   - status_led_init()        初始化一次（RMT 后端驱动 GPIO38 上的灯带）
 *   - status_led_set_rgb()     只点 index 0 那一颗、其余保持灭、自动按低亮度缩放
 *   - status_led_off()         全灭
 * 之后做待机/报警/心跳态时，直接在这之上加一层 status_led_set(STATE_xxx) 即可，
 * 不用重写驱动部分。
 *
 * 引脚与灯珠数复用 bsp_board.h 里已有的 LED_STRIP_GPIO_PIN / LED_STRIP_LED_COUNT。
 */

/* 全局亮度上限（0~255）。电池供电，刻意调低；传给 set_rgb 的颜色会按此缩放。 */
#define STATUS_LED_BRIGHTNESS   24

/* 初始化灯带（幂等：重复调用直接返回 ESP_OK）。 */
esp_err_t status_led_init(void);

/*
 * 只设置 index 0 那一颗的颜色，其余灯珠强制灭，然后刷新。
 * r/g/b 传 0~255 的「纯色」，函数内部按 STATUS_LED_BRIGHTNESS 缩放。
 * 例：status_led_set_rgb(255, 0, 0) → 低亮度红。
 */
esp_err_t status_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/* 全部灯珠熄灭。 */
esp_err_t status_led_off(void);

/*
 * 自检：让 index 0 循环 红→绿→蓝（每色约 0.5s），其余 6 颗保持灭。
 * 用于实测确认 GPIO38 + 驱动能点亮目标灯珠。此函数不返回（死循环）。
 */
void status_led_selftest_loop(void);

/* ── 录音固件的四个状态 ─────────────────────────────────────
 * 颜色映射（已和使用者确认）：
 *   待机/就绪      蓝 常亮
 *   SD 报警        红 ~5Hz 快闪（死循环停住）
 *   正在写卡心跳    绿 ~1Hz 闪（绑定真实 fwrite）
 *   收尾/正常关闭   绿 常亮
 */

/* 心跳节奏：写卡循环每次迭代约 32ms（512 帧@16kHz）；每累计 N 次「成功写入」
 * 翻转一次绿灯 → 半周期≈0.5s → 整体≈1Hz。改这个数即可调心跳快慢。 */
#define STATUS_LED_HEARTBEAT_TICKS   16

/* 待机：蓝常亮。 */
void status_led_standby(void);

/* SD 报警：红灯 ~5Hz 快闪，死循环不返回（挂载失败时调用，停在此态）。 */
void status_led_error_halt(void);

/* 进入一段录音：复位心跳计数，绿灯先点亮。 */
void status_led_record_begin(void);

/* 写卡心跳：录音循环每次迭代调一次，传入本次 fwrite 是否成功。
 * 仅在 write_ok 时累计计数，到节奏点翻转绿灯；write_ok=false 则不推进（自然冻结）。
 * 大多数调用只做一次自增后立即返回，不碰 RMT，不堵塞 I2S 热路径。 */
void status_led_record_tick(bool write_ok);

/* 收尾：文件正常关闭后绿灯常亮（保持多久由调用方控制）。 */
void status_led_recording_closed(void);

#ifdef __cplusplus
}
#endif
