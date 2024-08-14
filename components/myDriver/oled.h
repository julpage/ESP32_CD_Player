#ifndef __OLED_H
#define __OLED_H

#define OLED_ADDR 0x3c
#define OLED_CMD 0
#define OLED_DATA 1

#define Max_Column 128
#define Max_Row 64
#define Brightness 0xFF
#define X_WIDTH 128
#define Y_WIDTH 64

#define OLED_NUM_A 0x1
#define OLED_NUM_B 0x2
#define OLED_NUM_C 0x4
#define OLED_NUM_D 0x8
#define OLED_NUM_E 0x10
#define OLED_NUM_F 0x20
#define OLED_NUM_G 0x40

void OLED_WR_Byte(uint8_t dat, uint8_t cmd);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t reverseDisplay);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_progressBar(uint8_t y, float value);
void OLED_refreshScreen();

#endif
