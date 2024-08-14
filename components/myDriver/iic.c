#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "iic.h"

#include "main.h"

#define I2C_MASTER_SCL_IO PIN_I2C_SCL
#define I2C_MASTER_SDA_IO PIN_I2C_SDA
#define I2C_MASTER_DEV I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_TIMEOUT_MS 1000

SemaphoreHandle_t xMutex_iic;

void iic_init()
{
    xMutex_iic = xSemaphoreCreateMutex();

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_MASTER_DEV, &conf);
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_DEV, conf.mode, 0, 0, 0));
    ESP_LOGI("iic_init()", "I2C initialized successfully");
}

esp_err_t iic_writeBytes(uint8_t devAddr7bit, const uint8_t *buf, uint8_t len)
{
    xSemaphoreTake(xMutex_iic, portMAX_DELAY);
    esp_err_t ret;

    ret = i2c_master_write_to_device(
        I2C_MASTER_DEV,
        devAddr7bit, buf, len,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    xSemaphoreGive(xMutex_iic);
    return ret;
}

esp_err_t iic_readBytes(uint8_t devAddr7bit, uint8_t *buf, uint8_t len)
{
    xSemaphoreTake(xMutex_iic, portMAX_DELAY);
    esp_err_t ret;

    ret = i2c_master_read_from_device(
        I2C_MASTER_DEV,
        devAddr7bit, buf, len,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    xSemaphoreGive(xMutex_iic);
    return ret;
}

esp_err_t iic_writeReg(uint8_t devAddr7bit, uint8_t regAddr, const uint8_t *buf, uint8_t len)
{
    esp_err_t ret;

    uint8_t *dat = malloc(len + 1);
    dat[0] = regAddr;
    memcpy(dat + 1, buf, len);

    ret = iic_writeBytes(devAddr7bit, dat, len + 1);

    free(dat);

    return ret;
}

esp_err_t iic_readReg(uint8_t devAddr7bit, uint8_t regAddr, uint8_t *buf, uint8_t len)
{
    xSemaphoreTake(xMutex_iic, portMAX_DELAY);
    esp_err_t ret;

    ret = i2c_master_write_read_device(
        I2C_MASTER_DEV,
        devAddr7bit, &regAddr, 1,
        buf, len,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    xSemaphoreGive(xMutex_iic);
    return ret;
}