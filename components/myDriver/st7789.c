#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "main.h"
#include "st7789.h"

#define LCD_BL PIN_TFT_BL
#define LCD_DC PIN_TFT_DCX //  0:command 1:parameter RAM data
#define LCD_CS PIN_SPI_CS  // Chip select control pin
#define LCD_SCLK PIN_SPI_CLK
#define LCD_MOSI PIN_SPI_MOSI

spi_device_handle_t spiHandle;
uint32_t blDuty = 0;

void SPI_WriteByte(uint8_t *TxData, int len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = TxData,
    };
    spi_device_polling_transmit(spiHandle, &t);
}

void lcd_write_cmd(uint8_t cmd)
{
    gpio_set_level(LCD_DC, 0);
    SPI_WriteByte(&cmd, 1);
}

void lcd_write_data(uint8_t dat)
{
    gpio_set_level(LCD_DC, 1);
    SPI_WriteByte(&dat, 1);
}
void lcd_write_data_batch(uint8_t *dat, int len)
{
    gpio_set_level(LCD_DC, 1);
    SPI_WriteByte(dat, len);
}

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    y1 += 80;
    y2 += 80;

    uint8_t col[] = {
        x1 >> 8, x1 & 0xff,
        x2 >> 8, x2 & 0xff};
    uint8_t row[] = {
        y1 >> 8, y1 & 0xff,
        y2 >> 8, y2 & 0xff};
    lcd_write_cmd(0x2a);
    lcd_write_data_batch(col, 4);
    lcd_write_cmd(0x2b);
    lcd_write_data_batch(row, 4);
    lcd_write_cmd(0x2C); // Memory Write
}

void lcd_fill(uint32_t color)
{
    lcd_set_window(0, 0, LCD_W - 1, LCD_H - 1);

    uint8_t buf[LCD_W * 2];
    for (int i = 0; i < sizeof(buf); i++)
    {
        if (i % 2 == 0)
            buf[i] = color >> 8;
        else
            buf[i] = color & 0xff;
    }
    for (int i = 0; i < LCD_W * LCD_H * 2 / sizeof(buf); i++)
    {
        lcd_write_data_batch(buf, sizeof(buf));
    }
}

void lcd_init(void)
{
    // 初始化spi外设
    // init bus
    ESP_LOGI("lcd_init", "Initializing bus SPI%d...", SPI2_HOST + 1);
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = LCD_MOSI,
        .sclk_io_num = LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // 初始化spi驱动
    // init driver
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // 40MHz
        .mode = 0,                             // SPI mode 0
        .spics_io_num = LCD_CS,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spiHandle);

    // 重置lcd
    // reset lcd
    gpio_set_level(PIN_TFT_RST, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_TFT_RST, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);

    lcd_write_cmd(0x11);                  // Sleep out
    vTaskDelay(120 / portTICK_PERIOD_MS); // Delay 120ms

    lcd_write_cmd(0x36);
    lcd_write_data(
        //  my        mx         mv         ml         rgb        mh
        (1 << 7) | (1 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2));

    lcd_write_cmd(0x3A);
    lcd_write_data(0x05); // 16bit/pixel

    lcd_write_cmd(0xB2); // Porch Setting
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_cmd(0xB7); // Gate Control
    lcd_write_data(0x35);

    lcd_write_cmd(0xBB);  // VCOM Setting
    lcd_write_data(0x32); // Vcom=1.35V

    lcd_write_cmd(0xC2); // VDV and VRH Command Enable
    lcd_write_data(0x01);

    lcd_write_cmd(0xC3);  // VRH Set
    lcd_write_data(0x15); // GVDD=4.8V  颜色深度

    lcd_write_cmd(0xC4);  // VDV Set
    lcd_write_data(0x20); // VDV, 0x20:0v

    lcd_write_cmd(0xC6);  // Frame Rate Control in Normal Mode
    lcd_write_data(0x0F); // 0x0F:60Hz

    lcd_write_cmd(0xD0); // Power Control 1
    lcd_write_data(0xA4);
    lcd_write_data(0xA1); // avdd:6.8v avcl:-4.8v vds:2.3v

    lcd_write_cmd(0xE0); // Positive Voltage Gamma Control
    lcd_write_data(0xD0);
    lcd_write_data(0x08);
    lcd_write_data(0x0E);
    lcd_write_data(0x09);
    lcd_write_data(0x09);
    lcd_write_data(0x05);
    lcd_write_data(0x31);
    lcd_write_data(0x33);
    lcd_write_data(0x48);
    lcd_write_data(0x17);
    lcd_write_data(0x14);
    lcd_write_data(0x15);
    lcd_write_data(0x31);
    lcd_write_data(0x34);

    lcd_write_cmd(0xE1); // Negative Voltage Gamma Control
    lcd_write_data(0xD0);
    lcd_write_data(0x08);
    lcd_write_data(0x0E);
    lcd_write_data(0x09);
    lcd_write_data(0x09);
    lcd_write_data(0x15);
    lcd_write_data(0x31);
    lcd_write_data(0x33);
    lcd_write_data(0x48);
    lcd_write_data(0x17);
    lcd_write_data(0x14);
    lcd_write_data(0x15);
    lcd_write_data(0x31);
    lcd_write_data(0x34);
    lcd_write_cmd(0x21);

    lcd_setBrightness(80);

    lcd_disp(true);
}

void lcd_disp(bool en)
{
    if (en)
    {
        lcd_write_cmd(0x11);                  // Sleep out
        vTaskDelay(120 / portTICK_PERIOD_MS); // Delay 120ms
        lcd_write_cmd(0x29);                  // Display On
    }
    else
    {
        lcd_write_cmd(0x28); // Display Off
        lcd_write_cmd(0x10); // Sleep in
    }
}

// 0~100
void lcd_setBrightness(uint8_t brightness)
{
    uint32_t duty = (pow(2, LEDC_DUTY_RES) - 1) * brightness / 100;
    if (duty == blDuty)
        return;

    ESP_LOGI("lcd_setBrightness", "brightness %d", brightness);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);

    blDuty = duty;
}