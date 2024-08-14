#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "main.h"
#include "button.h"
#include "iic.h"
#include "i2s.h"
#include "usbhost_driver.h"
#include "cdPlayer.h"

void app_main(void)
{
    // init nvs
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        printf("NVS partition was truncated and needs to be erased\n%s\n", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /**
     * configure gpio
     */
    gpio_config_t io_conf;

    // output pin
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PIN_TFT_BL,
        .duty = 0, // Set duty to 0%
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << PIN_TFT_RST) | (1ULL << PIN_TFT_DCX);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // input pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << PIN_BTN_EJECT) | (1ULL << PIN_BTN_PLAY) | (1ULL << PIN_BTN_VOL_UP) |
                           (1ULL << PIN_BTN_VOL_DOWN) | (1ULL << PIN_BTN_NEXT) | (1ULL << PIN_BTN_PREVIOUS);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    /**
     * init driver
     */
    ESP_LOGI("app_main", "Init usb host");
    usbhost_driverInit();
    ESP_LOGI("app_main", "Init i2c");
    iic_init();
    ESP_LOGI("app_main", "Init i2s");
    i2s_init();
    ESP_LOGI("app_main", "Init button");
    btn_init();

    // player task
    ESP_LOGI("app_main", "Run cd player");
    cdplay_init();

    // gui task
    uint8_t oledTest = 0xae;
    err = iic_writeReg(0x3c, 0, &oledTest, 1);

    BaseType_t taskCreatRet;
    if (err != ESP_OK)
    {
        ESP_LOGI("app_main", "use st7789 as screen");
        taskCreatRet = xTaskCreatePinnedToCore(task_lvgl,
                                               "lvgl",
                                               10240,
                                               NULL,
                                               1,
                                               NULL,
                                               0);
        if (taskCreatRet == pdPASS)
            ESP_LOGI("app_main", "TaskCreate task_lvgl -> success");
        else
            ESP_LOGE("app_main", "TaskCreate task_lvgl -> fail");
    }
    else
    {
        ESP_LOGI("app_main", "use oled as screen");
        taskCreatRet = xTaskCreatePinnedToCore(task_oled,
                                               "oled",
                                               4096,
                                               NULL,
                                               1,
                                               NULL,
                                               0);
        if (taskCreatRet == pdPASS)
            ESP_LOGI("app_main", "TaskCreate task_oled -> success");
        else
            ESP_LOGE("app_main", "TaskCreate task_oled -> fail");
    }

    // while (1)
    // {
    //     char InfoBuffer[512];
    //     vTaskList((char *)&InfoBuffer);
    //     printf("\r\nName        State    Priority   Stack    Num\r\n");
    //     printf("%s\r\n", InfoBuffer);

    //     vTaskGetRunTimeStats((char *)&InfoBuffer);
    //     printf("\r\nName            Count           usage\r\n");
    //     printf("%s\r\n", InfoBuffer);

    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}
