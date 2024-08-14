#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "main.h"
#include "iic.h"
#include "oled.h"
#include "oledfont.h"

uint8_t oled_outBuf[8][128];

void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if (cmd == OLED_CMD)
        iic_writeReg(OLED_ADDR, 0x00, &dat, 1);
    else
        iic_writeReg(OLED_ADDR, 0x40, &dat, 1);
}

// 开启OLED显示
void OLED_Display_On(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
    OLED_WR_Byte(0X14, OLED_CMD); // DCDC ON
    OLED_WR_Byte(0XAF, OLED_CMD); // DISPLAY ON
}

// 关闭OLED显示
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
    OLED_WR_Byte(0X10, OLED_CMD); // DCDC OFF
    OLED_WR_Byte(0XAE, OLED_CMD); // DISPLAY OFF
}

// 坐标设置
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);                 // b+页地址
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD); // 8+列地址高位
    OLED_WR_Byte((x & 0x0f), OLED_CMD);               // 0+列之地低位
}

// 清屏函数
void OLED_Clear(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_Set_Pos(0, i);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(0x00, OLED_DATA);
    }
}

// 在指定位置显示一个字符串,包括部分字符
// x:0~127
// y:0~7
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t reverseDisplay)
{
    uint8_t c = 0, i = 0;
    while (*str != '\0')
    {
        if ((x + 6) > (Max_Column - 1))
        {
            x = 0;
            y++;
        }
        if (y > 7)
            return;

        c = *str - ' ';
        for (i = 0; i < 6; i++)
        {
            if (reverseDisplay)
                oled_outBuf[y][x] = ~F6x8[c][i];
            else
                oled_outBuf[y][x] = F6x8[c][i];
            x++;
        }
        str++;
    }
}

// 0.0~1.0
void OLED_progressBar(uint8_t y, float value)
{
    OLED_Set_Pos(0, y);
    oled_outBuf[y][0] = 0xff;
    oled_outBuf[y][1] = 0x81;
    for (int i = 0; i < 124; i++)
    {
        if (i < (124 * value))
            oled_outBuf[y][2 + i] = 0xbd;
        else
            oled_outBuf[y][2 + i] = 0x81;
    }
    oled_outBuf[y][126] = 0x81;
    oled_outBuf[y][127] = 0xff;
}

// 初始化SSD1306
void OLED_Init(void)
{
    OLED_WR_Byte(0xAE, OLED_CMD); //--display off
    OLED_WR_Byte(0x00, OLED_CMD); //---列地址低4位
    OLED_WR_Byte(0x10, OLED_CMD); //---列地址高4位
    OLED_WR_Byte(0x40, OLED_CMD); //--set start line address
    OLED_WR_Byte(0xB0, OLED_CMD); //--页地址
    OLED_WR_Byte(0x81, OLED_CMD); // contract control
    OLED_WR_Byte(0xf0, OLED_CMD); //--128
    OLED_WR_Byte(0xA1, OLED_CMD); // set segment remap
    OLED_WR_Byte(0xA6, OLED_CMD); //--a6正显，a7反显
    OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
    OLED_WR_Byte(0x3F, OLED_CMD); //--1/32 duty
    OLED_WR_Byte(0xC8, OLED_CMD); // c0或c8垂直翻转
    OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset
    OLED_WR_Byte(0x00, OLED_CMD); //

    OLED_WR_Byte(0xD5, OLED_CMD); // set osc division
    OLED_WR_Byte(0x80, OLED_CMD); //

    //     OLED_WR_Byte(0xD8, OLED_CMD); //set area color mode off
    //     OLED_WR_Byte(0x05, OLED_CMD); //

    OLED_WR_Byte(0xD9, OLED_CMD); // Set Pre-Charge Period
    OLED_WR_Byte(0xF1, OLED_CMD); //

    OLED_WR_Byte(0xDA, OLED_CMD); // set com pin configuartion
    OLED_WR_Byte(0x12, OLED_CMD); //

    OLED_WR_Byte(0xDB, OLED_CMD); // set Vcomh
    OLED_WR_Byte(0x30, OLED_CMD); //

    OLED_WR_Byte(0x8D, OLED_CMD); // set charge pump enable
    OLED_WR_Byte(0x14, OLED_CMD); //

    OLED_WR_Byte(0xAF, OLED_CMD); //--屏幕开关，af开，ae关

    OLED_Clear();
    memset(oled_outBuf, 0, 128 * 8);
}

void OLED_refreshScreen()
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_Set_Pos(0, i);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(oled_outBuf[i][n], OLED_DATA);
    }
    memset(oled_outBuf, 0, 128 * 8);
}