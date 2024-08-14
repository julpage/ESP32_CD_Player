#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_check.h"

#include "main.h"
#include "i2s.h"
#include "cdPlayer.h"

#define EXAMPLE_STD_BCLK_IO1 PIN_I2S_BCK // I2S bit clock io number
#define EXAMPLE_STD_WS_IO1 PIN_I2S_LRCK  // I2S word select io number
#define EXAMPLE_STD_DOUT_IO1 PIN_I2S_DAT // I2S data in io number

#define I2S_SAMPLE_RATE 44100
#define I2S_DATA_BIT I2S_DATA_BIT_WIDTH_16BIT
#define BYTES_PER_SAMPLE (I2S_DATA_BIT_WIDTH_16BIT / 8 * 2)

TaskHandle_t transmitTask;

i2s_chan_handle_t tx_chan;

uint8_t i2s_txBuf[I2S_BUF_NUM][I2S_TX_BUFFER_LEN];
uint8_t i2s_buf_sendI = 0;
uint8_t i2s_buf_inserI = 0;
volatile bool i2s_bufsEmpty = true;
volatile bool i2s_bufsFull = false;

// -60dB ~ 0dB
const float volumeScale[31] = {
    0.000000,
    0.001000, 0.001269, 0.001610, 0.002043, 0.002593, 0.003290, 0.004175, 0.005298, 0.006723, 0.008532,
    0.010826, 0.013738, 0.017433, 0.022122, 0.028072, 0.035622, 0.045204, 0.057362, 0.072790, 0.092367,
    0.117210, 0.148735, 0.188739, 0.239503, 0.303920, 0.385662, 0.489390, 0.621017, 0.788046, 1.000000};

void i2s_fillBuffer(uint8_t *dat)
{
    if (i2s_bufsFull)
        return;

    memcpy(i2s_txBuf[i2s_buf_inserI], dat, I2S_TX_BUFFER_LEN);

    i2s_buf_inserI = (i2s_buf_inserI + 1) % I2S_BUF_NUM;

    if (i2s_buf_inserI == i2s_buf_sendI)
        i2s_bufsFull = true;

    if (i2s_bufsEmpty)
    {
        i2s_bufsEmpty = false;
        ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
        vTaskResume(transmitTask);
    }
}

void i2s_transmitTask(void *args)
{
    if (i2s_buf_sendI == i2s_buf_inserI)
    {
        ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
        i2s_bufsEmpty = true;
        vTaskSuspend(transmitTask);
    }

    uint8_t *buf;
    static int downSampleCount = 0;
    static int64_t oL = 0;
    static int64_t oR = 0;
    while (1)
    {
        buf = i2s_txBuf[i2s_buf_sendI];

        // 发给示波器
        // sent to oscilloscope
        if (queue_oscilloscope != NULL)
        {
            ChannelValue_t oscilloscope;

            for (int i = 0; i < I2S_TX_BUFFER_LEN / 2; i += 2)
            {
                oL += ((int16_t *)buf)[i];
                oR += ((int16_t *)buf)[i + 1];
                downSampleCount++;

                if (downSampleCount == 20)
                {
                    oscilloscope.l = oL / downSampleCount;
                    oscilloscope.r = oR / downSampleCount;
                    xQueueSend(queue_oscilloscope, &oscilloscope, 0);
                    oL = 0;
                    oR = 0;
                    downSampleCount = 0;
                }
            }
        }

        // 音量处理
        // change volume
        float scale = volumeScale[cdplayer_playerInfo.volume];
        int16_t *sample = (int16_t *)(buf);
        for (int i = 0; i < (I2S_TX_BUFFER_LEN / 2); i++)
        {
            *sample = (int16_t)((float)(*sample) * scale);
            sample++;
        }

        if (i2s_channel_write(tx_chan, buf, I2S_TX_BUFFER_LEN, NULL, portMAX_DELAY) == ESP_OK)
        {
            memset(buf, 0, I2S_TX_BUFFER_LEN);
            i2s_bufsFull = false;

            i2s_buf_sendI = (i2s_buf_sendI + 1) % I2S_BUF_NUM;

            if (i2s_buf_sendI == i2s_buf_inserI)
            {
                ESP_LOGW("i2s_transmitTask", "I2S buffer empty");
                ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
                i2s_bufsEmpty = true;
                vTaskSuspend(transmitTask);
            }
        }
        else
        {
            printf("Write Task: i2s write failed\n");
        }
    }
}

void i2s_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_SLAVE);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        // .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT, I2S_SLOT_MODE_STEREO),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = EXAMPLE_STD_BCLK_IO1,
            .ws = EXAMPLE_STD_WS_IO1,
            .dout = EXAMPLE_STD_DOUT_IO1,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    BaseType_t taskCreatRet;
    taskCreatRet = xTaskCreatePinnedToCore(i2s_transmitTask,
                                           "i2s_transmitTask",
                                           4096,
                                           NULL,
                                           3,
                                           &transmitTask,
                                           1);
    if (taskCreatRet == pdPASS)
        ESP_LOGI("i2s_init", "\nTaskCreate i2s_transmitTask -> success");
    else
        ESP_LOGE("i2s_init", "\nTaskCreate i2s_transmitTask -> fail");
}