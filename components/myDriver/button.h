#ifndef __BUTTON_H_
#define __BUTTON_H_

#define BTN_EJECT 0
#define BTN_PLAY 1
#define BTN_VOL_UP 2
#define BTN_VOL_DOWN 3
#define BTN_NEXT 4
#define BTN_PREVIOUS 5

#define LONG_PRESS_THRESHOLD 1000

void btn_init();
void btn_renew(uint8_t noBuf);
uint8_t btn_getLevel(uint8_t pin);
uint8_t btn_getPosedge(uint8_t pin);
uint8_t btn_getNegedge(uint8_t pin);
uint8_t btn_getLongPress(uint8_t pin, uint8_t level);


#endif
