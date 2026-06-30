#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "usb_device_uac.h"

static const char *TAG = "UAC_I2S";

#define I2S_BCK_PIN     GPIO_NUM_1
#define I2S_WS_PIN      GPIO_NUM_2
#define I2S_DOUT_PIN    GPIO_NUM_3
#define I2S_MCLK_PIN    GPIO_NUM_4
#define SAMPLE_RATE     48000

static i2s_chan_handle_t s_i2s_tx = NULL;

/* 스피커 출력 콜백: USB에서 받은 PCM 데이터를 I2S로 보냄 */
static esp_err_t uac_output_cb(uint8_t *buf, size_t len, void *cb_ctx)
{
    if (!buf || len == 0) return ESP_OK;
    size_t written = 0;
    i2s_channel_write(s_i2s_tx, buf, len, &written, pdMS_TO_TICKS(100));
    return ESP_OK;
}

static void i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 240;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &s_i2s_tx, NULL));

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
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_i2s_tx, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_i2s_tx));
    ESP_LOGI(TAG, "I2S ready  BCK=%d WS=%d DOUT=%d", I2S_BCK_PIN, I2S_WS_PIN, I2S_DOUT_PIN);
}

static void uac_init(void)
{
    uac_device_config_t cfg = {
        .skip_tinyusb_init = false,
        .output_cb     = uac_output_cb,
        .input_cb      = NULL,
        .set_mute_cb   = NULL,
        .set_volume_cb = NULL,
        .cb_ctx        = NULL,
    };
    ESP_ERROR_CHECK(uac_device_init(&cfg));
    ESP_LOGI(TAG, "USB UAC ready — 48kHz/16-bit/Stereo");
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 Zero  USB->I2S->PCM5102  start");
    i2s_init();
    uac_init();
    while (1) {
        ESP_LOGI(TAG, "running...");
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
