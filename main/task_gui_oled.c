#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "main.h"
#include "iic.h"
#include "oled.h"
#include "cdPlayer.h"
#include "usbhost_driver.h"

void task_oled(void *args)
{
    OLED_Init();

    vTaskDelay(pdMS_TO_TICKS(123));

    char str[30];
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(50));

        // 光驱型号
        // drive model
        sprintf(str, "%s-%s", cdplayer_driveInfo.vendor, cdplayer_driveInfo.product);
        OLED_ShowString(0, 0, str, 0);

        // 碟状态
        // disc state
        if (usbhost_driverObj.deviceIsOpened == 0)
            OLED_ShowString(0, 1, "No drive", 0);
        else if (cdplayer_driveInfo.trayClosed == 0)
            OLED_ShowString(0, 1, "Tray open", 0);
        else if (cdplayer_driveInfo.discInserted == 0)
            OLED_ShowString(0, 1, "No disc", 0);
        else if (cdplayer_driveInfo.discIsCD == 0)
            OLED_ShowString(0, 1, "Not cdda", 0);
        else
            OLED_ShowString(0, 1, " Ready ", 1);

        // 音量
        // volume
        sprintf(str, "VOL: %02d", cdplayer_playerInfo.volume);
        OLED_ShowString(0, 5, str, 0);

        if (cdplayer_driveInfo.readyToPlay)
        {
            // 播放状态
            // play state
            if (cdplayer_playerInfo.fastForwarding)
                OLED_ShowString(95, 1, ">>>", 0);
            else if (cdplayer_playerInfo.fastBackwarding)
                OLED_ShowString(95, 1, "<<<", 0);
            else if (cdplayer_playerInfo.playing)
                OLED_ShowString(95, 1, "Play", 0);
            else
                OLED_ShowString(95, 1, "Pause", 0);

            // 碟名
            // disc title
            if (cdplayer_driveInfo.cdTextAvalibale)
                OLED_ShowString(0, 2, cdplayer_driveInfo.albumTitle, 0);

            // 轨名 轨号
            // playing track's number or title
            int8_t trackNum = cdplayer_playerInfo.playingTrackIndex;
            if (cdplayer_driveInfo.cdTextAvalibale)
            {
                sprintf(str,
                        "%-15.15s %02d/%02d",
                        cdplayer_driveInfo.trackList[trackNum].title,
                        trackNum + 1,
                        cdplayer_driveInfo.trackCount);
            }
            else
            {
                sprintf(str,
                        "Track %02d       %02d/%02d",
                        cdplayer_driveInfo.trackList[trackNum].trackNum,
                        trackNum + 1,
                        cdplayer_driveInfo.trackCount);
            }
            OLED_ShowString(0, 3, str, 0);

            // 歌手
            // performer
            if (cdplayer_driveInfo.cdTextAvalibale)
            {
                OLED_ShowString(0, 4, cdplayer_driveInfo.trackList[trackNum].performer, 0);
            }

            // 预加重
            // preEmphasis
            if (cdplayer_driveInfo.trackList[trackNum].preEmphasis)
                OLED_ShowString(100, 5, " PEM ", 1);

            // 播放时长
            // play time
            uint32_t readFrameCount = cdplayer_playerInfo.readFrameCount;
            uint32_t trackDuration = cdplayer_driveInfo.trackList[trackNum].trackDuration;
            if (cdplayer_driveInfo.readyToPlay)
            {
                hmsf_t now = cdplay_frameToHmsf(readFrameCount);
                hmsf_t duration = cdplay_frameToHmsf(trackDuration);
                sprintf(str, "%02d:%02d.%02d     %02d:%02d.%02d",
                        now.minute, now.second, now.frame,
                        duration.minute, duration.second, duration.frame);
                OLED_ShowString(0, 6, str, 0);
            }
            OLED_progressBar(7, ((float)readFrameCount) / ((float)trackDuration));
        }
        else
        {
            OLED_ShowString(0, 3, "X_X", 0);
            sprintf(str, "%02d:%02d.%02d     %02d:%02d.%02d",
                    0, 0, 0,
                    0, 0, 0);
            OLED_ShowString(0, 6, str, 0);
            OLED_progressBar(7, 0);
        }

        OLED_refreshScreen();
    }
}