#ifndef __I2S_H_
#define __I2S_H_

#define I2S_BUF_NUM 5
#define I2S_TX_BUFFER_SIZE_FRAME (8)
#define I2S_TX_BUFFER_LEN (2352 * I2S_TX_BUFFER_SIZE_FRAME)

extern uint8_t i2s_txBuf[I2S_BUF_NUM][I2S_TX_BUFFER_LEN];
extern volatile bool i2s_bufsFull;

void i2s_init();
void i2s_fillBuffer(uint8_t *dat);

#endif
