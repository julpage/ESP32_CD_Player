#ifndef __IIC_H_
#define __IIC_H_

void iic_init();
esp_err_t iic_writeBytes(uint8_t devAddr7bit, const uint8_t *buf, uint8_t len);
esp_err_t iic_readBytes(uint8_t devAddr7bit, uint8_t *buf, uint8_t len);
esp_err_t iic_writeReg(uint8_t devAddr7bit, uint8_t regAddr, const uint8_t *buf, uint8_t len);
esp_err_t iic_readReg(uint8_t devAddr7bit, uint8_t regAddr, uint8_t *buf, uint8_t len);

#endif
