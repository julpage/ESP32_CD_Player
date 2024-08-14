#ifndef __MAIN_H_
#define __MAIN_H_

#define PIN_BTN_EJECT 37
#define PIN_BTN_PLAY 36
#define PIN_BTN_VOL_UP 35
#define PIN_BTN_VOL_DOWN 1
#define PIN_BTN_NEXT 2
#define PIN_BTN_PREVIOUS 42
#define PIN_I2C_SDA 17
#define PIN_I2C_SCL 18
#define PIN_I2S_BCK 39
#define PIN_I2S_LRCK 41
#define PIN_I2S_DAT 40
#define PIN_TFT_BL 12
#define PIN_TFT_RST 48
#define PIN_TFT_DCX 13
#define PIN_SPI_CS 14
#define PIN_SPI_MOSI 47
#define PIN_SPI_CLK 21

#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_12_BIT

typedef struct
{
    int16_t l;
    int16_t r;
} ChannelValue_t;

extern QueueHandle_t queue_meter;
extern QueueHandle_t queue_oscilloscope;

void task_oled(void *args);
void task_lvgl(void *args);

#endif