#include <stdio.h>
// #include <time.h>
#include <sys/time.h>
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "main.h"
#include "button.h"

uint8_t btn_valueIn;
uint8_t btn_valueIn_pos;
uint8_t btn_valueIn_neg;
struct timeval edgeTime[6];

void btn_init()
{
    btn_valueIn = 0xff;
    btn_valueIn_pos = 0;
    btn_valueIn_neg = 0;
    btn_renew(1);
}

void btn_renew(uint8_t noBuf)
{
    uint8_t btn_valueIn_temp = 0;
    if (gpio_get_level(PIN_BTN_EJECT))
        btn_valueIn_temp |= 1 << BTN_EJECT;
    if (gpio_get_level(PIN_BTN_PLAY))
        btn_valueIn_temp |= 1 << BTN_PLAY;
    if (gpio_get_level(PIN_BTN_VOL_UP))
        btn_valueIn_temp |= 1 << BTN_VOL_UP;
    if (gpio_get_level(PIN_BTN_VOL_DOWN))
        btn_valueIn_temp |= 1 << BTN_VOL_DOWN;
    if (gpio_get_level(PIN_BTN_NEXT))
        btn_valueIn_temp |= 1 << BTN_NEXT;
    if (gpio_get_level(PIN_BTN_PREVIOUS))
        btn_valueIn_temp |= 1 << BTN_PREVIOUS;

    static uint8_t unstable = 0;
    static uint8_t valueIn_last = 0xff;

    if ((btn_valueIn_temp == unstable) || noBuf)
        btn_valueIn = btn_valueIn_temp;
    unstable = btn_valueIn_temp;

    // edge detect
    btn_valueIn_pos = (valueIn_last ^ btn_valueIn) & btn_valueIn;
    btn_valueIn_neg = (valueIn_last ^ btn_valueIn) & valueIn_last;

    int i;
    uint32_t edge = btn_valueIn_pos | btn_valueIn_neg;
    for (i = 0; i < 6; i++)
    {
        if ((edge & 0x01) == 0x01)
            gettimeofday(&edgeTime[i], NULL);
        edge >>= 1;
    }

    valueIn_last = btn_valueIn;
}

uint8_t btn_getLevel(uint8_t pin)
{
    uint8_t mask = (1 << pin);
    if ((btn_valueIn & mask) == mask)
        return 1;
    else
        return 0;
}

uint8_t btn_getPosedge(uint8_t pin)
{
    uint32_t mask = (1 << pin);
    if ((btn_valueIn_pos & mask) == mask)
        return 1;
    else
        return 0;
}

uint8_t btn_getNegedge(uint8_t pin)
{
    uint32_t mask = (1 << pin);
    if ((btn_valueIn_neg & mask) == mask)
        return 1;
    else
        return 0;
}

uint8_t btn_getLongPress(uint8_t pin, uint8_t level)
{
    if (btn_getLevel(pin) == level)
    {
        struct timeval now;
        gettimeofday(&now, NULL);

        uint32_t durationMs = (now.tv_sec - edgeTime[pin].tv_sec) * 1000 + (now.tv_usec - edgeTime[pin].tv_usec) / 1000;

        if (durationMs > LONG_PRESS_THRESHOLD)
            return 1;
        else
            return 0;
    }
    else
    {
        return 0;
    }
}
