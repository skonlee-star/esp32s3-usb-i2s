#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "usb_device_uac.h"

static const char *TAG = "uac_i2s";

/* ── GPIO 핀 (PCM5102A 연결) ── */
#define I2S_BCK_PIN     GPIO_NUM_1
#define I2S_WS_PIN      GPIO_NUM_2
#define I2S_DOUT_PIN    GPIO_NUM_3
#define I2S_MCLK_PIN    GPIO_NUM_4
#define SAMPLE_RATE     48000

static i2s_chan_handle_t s_tx_chan = NULL;

/* USB에서 받은 스피커 데이터 → I2S로 출력 */
static esp_err_t uac_device_output_cb(uint8_t *buf, size_t len, void *arg)
{
    if (!buf || len == 0) return ESP_OK;
    size_t written = 0;
    i2s_channel_write(s_tx_chan, buf, len, &written, pdMS_TO_TICKS(200));
    return ESP_OK;
}

/* 마이크 미사용 — 더미 콜백 */
static esp_err_t uac_device_input_cb(uint8_t *buf, size_t len,
                                     size_t *bytes_read, void *arg)
{
    memset(buf, 0, len);
    *bytes_read = len;
    return ESP_OK;
}

static void uac_device_set_mute_cb(uint32_t mute, void *arg)
{
    ESP_LOGI(TAG, "mute: %lu", mute);
}

static void uac_device_set_volume_cb(uint32_t volume, void *arg)
{
    ESP_LOGI(TAG, "volume: %lu", volume);
}

static void i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 240;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &s_tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_PIN,
            .bclk = I2S_BCK_PIN,
            .ws   = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din  = I2S_GPIO_UNUSED,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_tx_chan));
    ESP_LOGI(TAG, "I2S OK  BCK=%d WS=%d DOUT=%d",
             I2S_BCK_PIN, I2S_WS_PIN, I2S_DOUT_PIN);
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 Zero USB->I2S->PCM5102 start");

    i2s_init();

    uac_device_config_t config = {
        .output_cb     = uac_device_output_cb,
        .input_cb      = uac_device_input_cb,
        .set_mute_cb   = uac_device_set_mute_cb,
        .set_volume_cb = uac_device_set_volume_cb,
        .cb_ctx        = NULL,
    };
    ESP_ERROR_CHECK(uac_device_init(&config));
    ESP_LOGI(TAG, "USB UAC ready");

    while (1) {
        ESP_LOGI(TAG, "running...");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
