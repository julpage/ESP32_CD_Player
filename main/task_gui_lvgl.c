#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gptimer.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "main.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "st7789.h"
#include "usbhost_driver.h"
#include "cdPlayer.h"
#include "gui_cdPlayer.h"

QueueHandle_t queue_oscilloscope = NULL;

// 定时器回调
// timer interrupt handler
static bool IRAM_ATTR gpTimer_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    lv_tick_inc(5);
    return false;
}

void task_lvgl(void *args)
{
    queue_oscilloscope = xQueueCreate(500, sizeof(ChannelValue_t));

    ESP_LOGI("task_lvgl", "lcd_init");
    lcd_init();

    ESP_LOGI("task_lvgl", "lv_init");
    lv_init();

    // 创建定时器给lvgl用
    // create a timer
    ESP_LOGI("task_lvgl", "config gptimer.");
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 给定时器用的时钟频率 1MHz, 1 tick=1us
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = gpTimer_alarm_cb, // 回调函数
    };
    gptimer_alarm_config_t alarm_config2 = {
        .alarm_count = 4999,                // 触发中断事件的目标计数值 period = 5ms
        .reload_count = 0,                  // 中断发生时计数器设置成此数，auto_reload_on_alarm为1时有用
        .flags.auto_reload_on_alarm = true, // 是否使能自动重载功能
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config2));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    // 注册lvgl io设备
    // Register the display in LVGL
    ESP_LOGI("task_lvgl", "lv_port_disp_init.");
    lv_port_disp_init();

    // 绘制界面
    // draw ui
    ESP_LOGI("task_lvgl", "gui_player_init.");
    gui_player_init();

    char str[100];
    while (1)
    {
        // 光驱型号
        // cd drive model
        sprintf(str, "%s-%s", cdplayer_driveInfo.vendor, cdplayer_driveInfo.product);
        gui_setDriveModel(str);

        // 碟状态
        // disc state
        if (usbhost_driverObj.deviceIsOpened == 0)
            gui_setDriveState("No drive");
        else if (cdplayer_driveInfo.trayClosed == 0)
            gui_setDriveState("Tray open");
        else if (cdplayer_driveInfo.discInserted == 0)
            gui_setDriveState("No disc");
        else if (cdplayer_driveInfo.discIsCD == 0)
            gui_setDriveState("Not cdda");
        else
            gui_setDriveState("Ready");

        // 音量
        // volume
        gui_setVolume(cdplayer_playerInfo.volume);

        if (cdplayer_driveInfo.readyToPlay)
        {
            int8_t trackI = cdplayer_playerInfo.playingTrackIndex;

            // 播放状态
            // play state
            if (cdplayer_playerInfo.fastForwarding)
                gui_setPlayState(">>>");
            else if (cdplayer_playerInfo.fastBackwarding)
                gui_setPlayState("<<<");
            else if (cdplayer_playerInfo.playing)
                gui_setPlayState(LV_SYMBOL_PLAY);
            else
                gui_setPlayState(LV_SYMBOL_PAUSE);

            // 碟名
            // album title and performer
            if (cdplayer_driveInfo.cdTextAvalibale)
            {
                sprintf(str, "%s - %s", cdplayer_driveInfo.albumTitle, cdplayer_driveInfo.albumPerformer);
                gui_setAlbumTitle(str);
            }
            else
            {
                gui_setAlbumTitle("");
            }

            // 预加重
            // pre-emphasis
            if (cdplayer_driveInfo.trackList[trackI].preEmphasis)
                gui_setEmphasis(true);
            else
                gui_setEmphasis(false);

            // 轨名 歌手
            // track title and performer
            if (cdplayer_driveInfo.cdTextAvalibale)
            {
                gui_setTrackTitle(
                    cdplayer_driveInfo.trackList[trackI].title,
                    cdplayer_driveInfo.trackList[trackI].performer);
            }
            else
            {
                sprintf(str, "Track %02d", cdplayer_driveInfo.trackList[trackI].trackNum);
                gui_setTrackTitle(str, "");
            }

            // 播放时长
            // played time
            gui_setTime(
                cdplay_frameToHmsf(cdplayer_playerInfo.readFrameCount),
                cdplay_frameToHmsf(cdplayer_driveInfo.trackList[trackI].trackDuration));

            // 播放进度
            // play progress bar
            gui_setProgress(
                cdplayer_playerInfo.readFrameCount,
                cdplayer_driveInfo.trackList[trackI].trackDuration);

            // 轨号
            // track number
            gui_setTrackNum(trackI + 1, cdplayer_driveInfo.trackCount);

            // 示波器 电平表
            // oscilloscope and level meter
            static int count = 0;
            static int32_t maxL = 0;
            static int32_t maxR = 0;
            ChannelValue_t oscilloscopePoint;
            while (xQueueReceive(queue_oscilloscope, &oscilloscopePoint, 0) == pdTRUE)
            {
                // lv_chart_set_next_value(chart_left, ser_left, oscilloscopePoint.l);
                // lv_chart_set_next_value(chart_right, ser_right, oscilloscopePoint.r);
                int pointCount = ((lv_chart_t *)chart_left)->point_cnt;
                for (int i = 0; i < pointCount - 1; i++)
                {
                    ser_left->y_points[i] = ser_left->y_points[i + 1];
                    ser_right->y_points[i] = ser_right->y_points[i + 1];
                }
                ser_left->y_points[pointCount - 1] = oscilloscopePoint.l;
                ser_right->y_points[pointCount - 1] = oscilloscopePoint.r;

                if (oscilloscopePoint.l > maxL)
                    maxL = oscilloscopePoint.l;
                if (oscilloscopePoint.r > maxR)
                    maxR = oscilloscopePoint.r;

                if ((++count) == 100)
                {
                    count = 0;
                    lv_chart_refresh(chart_left);
                    lv_chart_refresh(chart_right);

                    double dbL = 20 * log10(maxL);
                    double dbR = 20 * log10(maxR);
                    maxL = 0;
                    maxR = 0;

                    gui_setMeter(dbL, dbR);
                    // gui_setMeter(maxR, maxL);
                    break;
                }
            }
        }
        else
        {
            gui_setPlayState(LV_SYMBOL_STOP);
            gui_setAlbumTitle("");
            gui_setEmphasis(false);
            gui_setTrackTitle("(=^_^=)", "");
            gui_setTime(cdplay_frameToHmsf(0), cdplay_frameToHmsf(0));
            gui_setProgress(0, 0);
            gui_setTrackNum(0, 0);
            gui_setMeter(0, 0);
            lv_chart_set_all_value(chart_left, ser_left, 0);
            lv_chart_set_all_value(chart_right, ser_right, 0);
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}