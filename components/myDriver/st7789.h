#ifndef __ST7789_H_
#define __ST7789_H_

#define LCD_W 240
#define LCD_H 240

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_write_data_batch(uint8_t *dat, int len);
void lcd_fill(uint32_t color);
void lcd_drawPoint(uint16_t x, uint16_t y, uint32_t color);

void lcd_init();

void lcd_disp(bool en);
void lcd_setBrightness(uint8_t brightness);



#endif