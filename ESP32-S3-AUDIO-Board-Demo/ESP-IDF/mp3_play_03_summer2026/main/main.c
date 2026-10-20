#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>     // fsync / fileno
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"      // esp_timer_get_time（消抖/冷却按墙钟计时）
#include "driver/gpio.h"
#include "bsp_board.h"
#include "status_led.h"

static const char *TAG = "REC";

/* ─── 录音参数（与诊断版一致，先不动）────────────────── */
#define SAMPLE_RATE       16000
#define CHANNELS          4
#define CAPTURE_BITS      16      /* ES7210 在 TDM 4ch 模式下每样本就是 16-bit（4 路打包进 64bit 帧），不是 32-bit */
#define OUTPUT_BITS       16      /* 写入 SD + WAV header 声明的位深 */

#define FRAMES_PER_CALL   512     /* 每次读 4096B ÷ (4ch×16bit = 8B/帧) = 512 帧 */
#define SAMPLES_PER_CALL  (FRAMES_PER_CALL * CHANNELS)              /* 2048 个样本（旧版的 2 倍） */
#define READ_BYTES        (SAMPLES_PER_CALL * (CAPTURE_BITS / 8))   /* 4096：从 codec 读（4ch×16bit 打包，块大小不变） */
#define WRITE_BYTES       (SAMPLES_PER_CALL * (OUTPUT_BITS / 8))    /* 4096：原样写 SD，不再丢弃任何样本 */

/* 录音中每隔这么多秒，用当前已写字节数回填一次 WAV header（断电/忘按停也基本可播）。 */
#define FLUSH_INTERVAL_SECONDS   5

/* ── BOOT 按键（GPIO0）：边沿检测 + 消抖 + 启动冷却 ──────────
 * 低电平=按下（带上拉）。用墙钟计时，与轮询节奏无关，
 * 待机(10ms 轮询)和录音循环(~32ms/buffer)两种节奏下消抖时间都准。 */
#define BTN_GPIO          GPIO_NUM_0
#define BTN_POLL_MS       10               /* 待机等待第一次按键时的轮询周期 */
#define BTN_DEBOUNCE_US   (30 * 1000)      /* 电平需稳定保持 30ms 才采纳，滤机械抖动 */
#define START_COOLDOWN_US (1000 * 1000)    /* 启动录音后 1s 内不接受停止键，避开同一次按压的松开抖动 */

static int g_clip_index = 1;

/* ─── WAV 文件头 ───────────────────────────────────────── */
#pragma pack(push, 1)
typedef struct {
    char     riff_id[4];
    uint32_t riff_size;
    char     wave_id[4];
    char     fmt_id[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_id[4];
    uint32_t data_size;
} wav_header_t;
#pragma pack(pop)

static void write_wav_header(FILE *f, uint32_t data_bytes)
{
    wav_header_t h = {
        .riff_id         = {'R','I','F','F'},
        .riff_size       = data_bytes + sizeof(wav_header_t) - 8,
        .wave_id         = {'W','A','V','E'},
        .fmt_id          = {'f','m','t',' '},
        .fmt_size        = 16,
        .audio_format    = 1,
        .num_channels    = CHANNELS,
        .sample_rate     = SAMPLE_RATE,
        .byte_rate       = SAMPLE_RATE * CHANNELS * (OUTPUT_BITS / 8),
        .block_align     = CHANNELS * (OUTPUT_BITS / 8),
        .bits_per_sample = OUTPUT_BITS,
        .data_id         = {'d','a','t','a'},
        .data_size       = data_bytes,
    };
    fwrite(&h, sizeof(h), 1, f);
}

/* ─── WAV header 收尾（唯一一份，两处复用）───────────────────
 * 把当前已写的音频字节数回填进文件头(data_size/riff_size)，并强制落盘。
 *   - 录音中每 5s 定期调用 → 断电/忘按停也基本可播
 *   - 第二次按 BOOT 停止时调用 → 保证最终 header 字节数正确（文件不废）
 * 写完 header 后 fseek 回文件末尾：后续音频继续顺序追加，不打断数据连续性。 */
static void finalize_wav_header(FILE *f, uint32_t data_bytes)
{
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_bytes);
    fseek(f, 0, SEEK_END);
    fflush(f);
    fsync(fileno(f));
}

/* ─── 按键工具 ─────────────────────────────────────────── */
static void button_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
}

/* 边沿 + 消抖：每次调用采一次 GPIO0，只有当电平「稳定保持 ≥BTN_DEBOUNCE_US」
 * 后才采纳为新的确认电平；在确认电平发生 高→低（松开→按下）跳变的那一刻返回一次
 * true，即「一个干净的按下事件」。松开(低→高)只更新确认电平、不返回 true。
 * 内部状态跨调用保留，所以待机等待和录音循环里可以共用同一个检测器，
 * 一次物理按压最多只产生一个按下事件。用墙钟计时，轮询快慢都不影响消抖时长。 */
static bool button_check_press(void)
{
    static int     confirmed = 1;   /* 已确认的稳定电平：1=松开，0=按下。开机假定松开 */
    static int     candidate = 1;   /* 正在计时的候选电平 */
    static int64_t candidate_since = 0;

    int     raw = gpio_get_level(BTN_GPIO);   /* 0=按下 */
    int64_t now = esp_timer_get_time();

    if (raw != candidate) {
        candidate = raw;                       /* 电平刚变，重新计稳定时间 */
        candidate_since = now;
        return false;
    }
    if (candidate != confirmed && (now - candidate_since) >= BTN_DEBOUNCE_US) {
        confirmed = candidate;                 /* 采纳新的稳定电平 */
        return (confirmed == 0);               /* 确认的下降沿 = 一次按下事件 */
    }
    return false;
}

/* 待机：以 BTN_POLL_MS 轮询，直到检测到一个消抖后的按下事件才返回。 */
static void wait_for_start_press(void)
{
    while (!button_check_press()) {
        vTaskDelay(pdMS_TO_TICKS(BTN_POLL_MS));
    }
}

/* ─── 录一段（持续写卡，直到第二次按 BOOT → 停）──────────── */
static void record_one_clip(void)
{
    char path[32];
    snprintf(path, sizeof(path), "/sdcard/rec_%03d.wav", g_clip_index);

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "无法创建文件: %s（SD卡挂载了吗？）", path);
        status_led_error_halt();   /* SD 失效，红灯快闪 + 停住，需重启（不返回） */
    }

    wav_header_t placeholder = {0};
    fwrite(&placeholder, sizeof(placeholder), 1, f);

    static int16_t in_buf[SAMPLES_PER_CALL];   /* 直接按 16-bit 读 TDM 打包样本 */
    static int16_t out_buf[SAMPLES_PER_CALL];  /* 4 声道交错，原样写出 */
    uint32_t total_bytes = 0;
    uint32_t frames_done = 0;
    bool sd_write_failed = false;

    ESP_LOGI(TAG, "▶ 开始录音 #%d → %s（持续录，再按一次 BOOT 停止）",
             g_clip_index, path);

    status_led_record_begin();   /* 心跳：绿灯亮起、计数复位 */
    int64_t record_start_us = esp_timer_get_time();   /* 冷却计时起点 */

    while (1) {
        /* 停止：边沿+消抖的第二次 BOOT。启动后 1s 冷却内忽略按下事件，
           避免同一次物理按压的松开抖动被误判成停止。 */
        if (button_check_press()) {
            if (esp_timer_get_time() - record_start_us >= START_COOLDOWN_US) {
                ESP_LOGI(TAG, "⏹ 收到第二次 BOOT，停止录音");
                break;
            }
            ESP_LOGW(TAG, "冷却期内(<1s)的按键已忽略");
        }

        esp_err_t ret = esp_get_feed_data(true, in_buf, READ_BYTES);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_get_feed_data 出错: %s", esp_err_to_name(ret));
            break;
        }

        /* 流本身就是 4 声道 × 16-bit 交错，正是 WAV data 需要的格式：直接搬运。
           不再 >>16——之前那次右移把每个 int32 里打包的第二个声道整丢，
           同时把样本数砍半，正是“每声道掉到 8kHz 且只剩 2 路”的根因。 */
        for (int i = 0; i < SAMPLES_PER_CALL; i++) {
            out_buf[i] = in_buf[i];
        }

        size_t written = fwrite(out_buf, 1, WRITE_BYTES, f);
        if (written != WRITE_BYTES) {
            ESP_LOGE(TAG, "SD卡写入不完整: %u/%d", (unsigned)written, WRITE_BYTES);
            sd_write_failed = true;   /* 先 break 去收尾段 salvage，再 latch 红 */
            break;
        }

        total_bytes += WRITE_BYTES;
        frames_done += FRAMES_PER_CALL;

        /* 心跳：每次成功写一个 buffer 推进一次，内部约每16次翻转绿灯(~1Hz)。
           只在“每写一个 buffer”的节奏上调用，不进上面的每样本搬运循环。 */
        status_led_record_tick(written == WRITE_BYTES);

        /* ★ 可靠性：每 FLUSH_INTERVAL_SECONDS 秒回填 header + 强制落盘，
           断电/忘按停也能直接打开。只发生在「刚写完一整个 buffer」的迭代边界上，
           不会插进音频数据中间；finalize 内部写完 header 会 seek 回末尾，
           下一次 fwrite 仍是顺序追加，数据连续性不受影响。 */
        if ((frames_done % (SAMPLE_RATE * FLUSH_INTERVAL_SECONDS)) < FRAMES_PER_CALL) {
            finalize_wav_header(f, total_bytes);
            ESP_LOGI(TAG, "已录 %lu 秒（header 已刷新）",
                     (unsigned long)(frames_done / SAMPLE_RATE));
        }
    }

    /* 收尾：复用同一份 finalize，把最终字节数写回 header 再关闭。 */
    finalize_wav_header(f, total_bytes);
    fclose(f);

    /* SD 写失败：已尽力把当前文件收尾(上面的回填+fclose)，再 latch 红灯停住。 */
    if (sd_write_failed) {
        ESP_LOGE(TAG, "SD写失败，已尽力收尾当前文件，进入红灯报警(需重启)");
        status_led_error_halt();   /* 不返回 */
    }

    status_led_recording_closed();   /* 收尾：文件已正常关闭，绿灯常亮 */

    ESP_LOGI(TAG, "✓ 已保存 %s（%.2f MB）",
             path, (float)total_bytes / (1024.0f * 1024.0f));
    g_clip_index++;
}

/* ─── 开机扫描 SD 卡，找已有的最大编号，避免覆盖 ─── */
static void scan_existing_clips(void)
{
    int idx = 1;
    char path[32];
    while (1) {
        snprintf(path, sizeof(path), "/sdcard/rec_%03d.wav", idx);
        FILE *f = fopen(path, "rb");
        if (f) {
            fclose(f);
            idx++;
        } else {
            break;
        }
    }
    g_clip_index = idx;
    ESP_LOGI(TAG, "扫描完成，下一段从 rec_%03d 开始", g_clip_index);
}

/* ─── 主程序：初始化一次 → 循环（等按键→录一段）─────────── */
void app_main(void)
{
    ESP_LOGI(TAG, "=== 录音程序启动 ===");

    ESP_ERROR_CHECK(status_led_init());   /* 状态灯先就绪，后续各态才能显示 */

    ESP_ERROR_CHECK(esp_board_init(SAMPLE_RATE, 1, 16));
    ESP_LOGI(TAG, "板卡初始化完成");

    /* SD 挂载失败 → 红灯快闪并停住（软处理，不再 panic 重启）。 */
    esp_err_t sd_ret = esp_sdcard_init("/sdcard", 5);
    if (sd_ret != ESP_OK) {
        ESP_LOGE(TAG, "SD卡挂载失败: %s —— 进入红灯报警态", esp_err_to_name(sd_ret));
        status_led_error_halt();   /* 死循环，不返回 */
    }
    ESP_LOGI(TAG, "SD卡挂载完成");

    int feed_ch = esp_get_feed_channel();
    char *fmt   = esp_get_input_format();
    ESP_LOGI(TAG, "声道数: %d  格式: %s", feed_ch, fmt ? fmt : "NULL");

    scan_existing_clips();
    button_init();
    ESP_LOGI(TAG, "就绪。按 BOOT 键开始录音。");

    while (1) {
        status_led_standby();              /* 待机：蓝常亮 */
        wait_for_start_press();            /* 第一次 BOOT（边沿+消抖）→ 开录 */
        record_one_clip();                 /* 持续录：绿心跳 → 第二次 BOOT 收尾绿常亮 */
        vTaskDelay(pdMS_TO_TICKS(1000));   /* 收尾绿保持约1秒，再回待机 */
        ESP_LOGI(TAG, "等待下一段… 按 BOOT 继续。");
    }
}