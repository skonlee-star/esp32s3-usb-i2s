/*
 * ESP32-S3 Zero  —  USB Audio Class (UAC) → I2S → PCM5102A DAC
 *
 * 핀 배정 (PCM5102A 연결):
 *   GPIO 1  →  BCK  (Bit Clock)
 *   GPIO 2  →  LCK  (Word Select / LRCK)
 *   GPIO 3  →  DIN  (Data In to DAC)
 *   GPIO 4  →  SCK  (System Clock / MCLK — PCM5102 자체 생성 사용 시 NC 가능)
 *
 * PC에서 USB 사운드카드로 인식됩니다.
 * 지원: 48kHz / 16-bit / Stereo
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "usb_device_uac.h"

static const char *TAG = "UAC_I2S";

/* ── GPIO 핀 정의 ─────────────────────────────────────────────── */
#define I2S_BCK_PIN     GPIO_NUM_1   /* BCK  → PCM5102 BCK  */
#define I2S_WS_PIN      GPIO_NUM_2   /* LRCK → PCM5102 LCK  */
#define I2S_DOUT_PIN    GPIO_NUM_3   /* DOUT → PCM5102 DIN  */
#define I2S_MCLK_PIN    GPIO_NUM_4   /* MCLK (PCM5102는 내부 PLL, NC 가능) */

/* ── 오디오 파라미터 ────────────────────────────────────────────── */
#define SAMPLE_RATE     48000
#define BIT_WIDTH       I2S_DATA_BIT_WIDTH_16BIT
#define CHANNELS        2            /* Stereo */
#define SPK_BUF_SIZE    (32 * 1024)  /* 링버퍼 32KB */

/* ── I2S 핸들 ──────────────────────────────────────────────────── */
static i2s_chan_handle_t s_i2s_tx = NULL;

/* ── UAC 스피커 콜백: USB에서 수신한 PCM을 I2S로 전송 ─────────── */
static esp_err_t spk_data_cb(uint8_t *buf, size_t len, void *ctx)
{
    if (!buf || len == 0) return ESP_OK;

    size_t written = 0;
    esp_err_t ret = i2s_channel_write(s_i2s_tx, buf, len, &written,
                                      pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "i2s_write err: %d (wrote %u/%u)", ret,
                 (unsigned)written, (unsigned)len);
    }
    return ESP_OK;
}

/* ── I2S 초기화 ────────────────────────────────────────────────── */
static void i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 240;   /* 5 ms @ 48kHz */

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &s_i2s_tx, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            BIT_WIDTH, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_PIN,
            .bclk = I2S_BCK_PIN,
            .ws   = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_i2s_tx, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_i2s_tx));

    ESP_LOGI(TAG, "I2S ready: %d Hz / 16-bit / Stereo", SAMPLE_RATE);
    ESP_LOGI(TAG, "  BCK  → GPIO%d", I2S_BCK_PIN);
    ESP_LOGI(TAG, "  LRCK → GPIO%d", I2S_WS_PIN);
    ESP_LOGI(TAG, "  DOUT → GPIO%d", I2S_DOUT_PIN);
    ESP_LOGI(TAG, "  MCLK → GPIO%d (PCM5102 내부 PLL 사용 시 미연결 가능)",
             I2S_MCLK_PIN);
}

/* ── UAC 초기화 ─────────────────────────────────────────────────── */
static void uac_init(void)
{
    uac_device_config_t cfg = {
        .output_ch          = CHANNELS,
        .input_ch           = 0,          /* 마이크 미사용 */
        .output_sample_rate = SAMPLE_RATE,
        .input_sample_rate  = SAMPLE_RATE,
        .spk_buf_size       = SPK_BUF_SIZE,
        .spk_new_play_cb    = spk_data_cb,
        .spk_new_play_ctx   = NULL,
    };

    ESP_ERROR_CHECK(uac_device_init(&cfg));
    ESP_LOGI(TAG, "USB UAC device ready (48kHz/16-bit/Stereo)");
    ESP_LOGI(TAG, "PC에서 'ESP32-S3 UAC Speaker'로 인식됩니다");
}

/* ── app_main ────────────────────────────────────────────────────── */
void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 Zero  USB → I2S → PCM5102A  시작");

    i2s_init();
    uac_init();

    /* 실행 중 상태 출력 (30초마다) */
    while (1) {
        ESP_LOGI(TAG, "동작 중... (USB 오디오 스트리밍 대기)");
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
