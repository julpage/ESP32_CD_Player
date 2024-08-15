#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "usbhost_scsi_cmd.h"
#include "cdPlayer.h"
#include "button.h"
#include "i2s.h"

cdplayer_driveInfo_t cdplayer_driveInfo;
cdplayer_playerInfo_t cdplayer_playerInfo;
uint8_t readCdBuf[I2S_TX_BUFFER_LEN];

void printMem(uint8_t *dat, uint16_t size)
{
    for (int i = 0; i < size; i++)
        printf("%02x ", dat[i]);
    printf("\n");
}

esp_err_t cdplayer_discReady(uint8_t *ready, uint8_t *trayClosed)
{
    esp_err_t err = usbhost_scsi_testUnitReady();

    uint8_t senseDat[18];
    if (err != ESP_OK)
    {
        *ready = 0;
        *trayClosed = 1;

        usbhost_scsi_requestSense(senseDat);
        uint8_t asc = senseDat[12];
        uint8_t ascq = senseDat[13];
        if (asc == 0x3a && ascq == 0x02)
            *trayClosed = 0;
    }
    else
    {
        *ready = 1;
        *trayClosed = 1;
    }
    return err;
}

bool cdplayer_discIsCd()
{
    uint8_t senseDat[18];
    esp_err_t err;

    uint8_t requireDat[20];
    uint32_t requireDatLen;

    // 判断cd-rom特性是否激活
    // is cd driver in cd-rom mode
    requireDatLen = 8;
    err = usbhost_scsi_getConfiguration(0x0000, 0x2, requireDat, &requireDatLen); // get Profile List Feature (0000h)
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat); // i don't care what cause fail
        return false;
    }
    uint16_t currentProfile = __builtin_bswap16(*(uint16_t *)(requireDat + 6));
    if (currentProfile != 0x0008) // CD-ROM, MMC-4 Table 78 - Profile List
        return false;

    // 判断碟是否为cd-da或cd-rom
    // is disc is cd-da or cd-rom
    requireDatLen = 9;
    err = usbhost_scsi_readDiscInformation(requireDat, &requireDatLen);
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat);
        return false;
    }
    if (requireDat[8] != 0x00) // 00h：CD-DA or CD-ROM Disc, MMC-4 Table 361 - Disc Information Block
        return false;

    return true;
}

esp_err_t cdplayer_getPlayList(uint8_t *tracksCount, cdplayer_trackInfo_t *trackList)
{
    uint8_t senseDat[18];
    esp_err_t err;
    uint8_t *tocDat;

    //
    // 预读toc长度
    // get toc length
    //
    uint32_t requireDatLen;
    tocDat = (uint8_t *)malloc(4);
    if (tocDat == NULL)
    {
        ESP_LOGE("cdplayer_getPlayList", "malloc fail");
        return ESP_FAIL;
    }

    requireDatLen = 4;
    err = usbhost_scsi_readTOC(false, 0, tocDat, &requireDatLen); // Format 0000b: Formatted TOC
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat);
        uint8_t senseKey = senseDat[2] & 0x0f;
        uint8_t asc = senseDat[12];
        uint8_t ascq = senseDat[13];
        ESP_LOGE("cdplayer_getPlayList", "Get toc length fail, senseKey:%02x ASC:%02x%02x", senseKey, asc, ascq);
        free(tocDat);
        return ESP_FAIL;
    }
    uint32_t tocLen = __builtin_bswap16(((usbhost_scsi_tocHeader_t *)tocDat)->TOC_Data_Length) + 2;
    free(tocDat);

    //
    // 获取全部toc
    // get all toc data
    //
    tocDat = (uint8_t *)malloc(tocLen);
    if (tocDat == NULL)
    {
        ESP_LOGE("cdplayer_getPlayList", "malloc fail");
        return ESP_FAIL;
    }

    requireDatLen = tocLen;
    err = usbhost_scsi_readTOC(false, 0, tocDat, &requireDatLen); // Format 0000b: Formatted TOC
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat);
        uint8_t senseKey = senseDat[2] & 0x0f;
        uint8_t asc = senseDat[12];
        uint8_t ascq = senseDat[13];
        ESP_LOGE("cdplayer_getPlayList", "Get full toc fail, senseKey:%02x ASC:%02x%02x", senseKey, asc, ascq);
        free(tocDat);
        return ESP_FAIL;
    }

    //
    // 分析toc
    // analyze toc data
    //
    usbhost_scsi_tocHeader_t *header = (usbhost_scsi_tocHeader_t *)(tocDat);
    usbhost_scsi_tocTrackDesriptor_t *tocDesc = (usbhost_scsi_tocTrackDesriptor_t *)(tocDat + 4);
    *tracksCount = 0;

    uint16_t tocDataSize = __builtin_bswap16(header->TOC_Data_Length) - 2;
    bool previousTrackIsNotAudio = false;
    for (int i = 0; i < tocDataSize / 8; i++)
    {
        // 计算前一轨时长
        // get previous track duration
        uint32_t trackStartAddress = __builtin_bswap32(tocDesc->Track_Start_Address);
        if (*tracksCount > 0 && !previousTrackIsNotAudio)
            trackList[*tracksCount - 1].trackDuration = trackStartAddress - trackList[*tracksCount - 1].lbaBegin;

        // 如果当前轨不是音频轨就跳过
        // if this track is not audio track, skip
        if ((tocDesc->ADR_CONTROL & 0x0c) != 0x00)
        {
            previousTrackIsNotAudio = true;
            continue;
        }

        // 如果是最后一条轨就结束循环
        // aa is the last track, break
        if (tocDesc->Track_Number == 0xaa)
            break;

        trackList[*tracksCount].trackNum = tocDesc->Track_Number;
        trackList[*tracksCount].lbaBegin = trackStartAddress;
        trackList[*tracksCount].preEmphasis = tocDesc->ADR_CONTROL & 0x01;
        trackList[*tracksCount].trackDuration = 0;
        trackList[*tracksCount].title = NULL;
        trackList[*tracksCount].performer = NULL;
        (*tracksCount)++;
        tocDesc++;
        previousTrackIsNotAudio = false;
    }

    free(tocDat);
    return ESP_OK;
}

esp_err_t cpplayer_getCdText(char **albumTitle, char **albumPerformer, char **titleStrBuf, char **performerStrBuf, uint8_t tracksCount, cdplayer_trackInfo_t *trackList)
{
    uint8_t senseDat[18];
    esp_err_t err;
    uint32_t requireDatLen;
    uint8_t *cdTextDat;

    if (*titleStrBuf != NULL)
        free(*titleStrBuf);
    if (*performerStrBuf != NULL)
        free(*performerStrBuf);

    //
    // 检查光驱能否读cd text
    // check if cd driver can read cd text
    //
    uint8_t requireDat[20];
    requireDatLen = 16;
    err = usbhost_scsi_getConfiguration(0x001e, 0x2, requireDat, &requireDatLen); // get CD Read Feature (001Eh)
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat);
        return err;
    }
    if (requireDatLen <= 8) // feature not support or not current
        return ESP_FAIL;
    if ((requireDat[12] & 0x01) != 1) // CD-Text is not supported
        return ESP_FAIL;

    //
    // 预读cdtext长度
    // get cd-text length
    //
    cdTextDat = (uint8_t *)malloc(4);
    if (cdTextDat == NULL)
    {
        ESP_LOGE("cpplayer_getCdText", "malloc fail");
        return ESP_FAIL;
    }

    requireDatLen = 4;
    err = usbhost_scsi_readTOC(false, 5, cdTextDat, &requireDatLen); // Format 0101b: CD-TEXT
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat);
        uint8_t senseKey = senseDat[2] & 0x0f;
        uint8_t asc = senseDat[12];
        uint8_t ascq = senseDat[13];
        ESP_LOGE("cpplayer_getCdText", "Get CD-TEXT length fail, senseKey:%02x ASC:%02x%02x", senseKey, asc, ascq);
        free(cdTextDat);
        return ESP_FAIL;
    }
    uint32_t tocLen = __builtin_bswap16(((usbhost_scsi_tocHeader_t *)cdTextDat)->TOC_Data_Length) + 2;
    free(cdTextDat);

    //
    // 获取全部cdtext
    // get cd-text
    //
    cdTextDat = (uint8_t *)malloc(tocLen);
    if (cdTextDat == NULL)
    {
        ESP_LOGE("cpplayer_getCdText", "malloc fail");
        return ESP_FAIL;
    }

    requireDatLen = tocLen;
    err = usbhost_scsi_readTOC(false, 5, cdTextDat, &requireDatLen); // Format 0101b: CD-TEXT
    if (err != ESP_OK)
    {
        usbhost_scsi_requestSense(senseDat);
        uint8_t senseKey = senseDat[2] & 0x0f;
        uint8_t asc = senseDat[12];
        uint8_t ascq = senseDat[13];
        ESP_LOGE("cpplayer_getCdText", "Get full CD-TEXT, senseKey:%02x ASC:%02x%02x", senseKey, asc, ascq);
        free(cdTextDat);
        return ESP_FAIL;
    }

    //
    // 分析字符串
    // split cd text
    //
    usbhost_scsi_tocHeader_t *header = (usbhost_scsi_tocHeader_t *)(cdTextDat);
    usbhost_scsi_tocCdTextDesriptor_t *textSequence = (usbhost_scsi_tocCdTextDesriptor_t *)(cdTextDat + 4);

    // 检查cd text长度，18的倍数
    // check cd text descriptor length
    uint16_t cdTextDescSize = __builtin_bswap16(header->TOC_Data_Length) - 2;
    if (cdTextDescSize % 18 != 0)
    {
        ESP_LOGE("cpplayer_getCdText", "CD-TEXT descriptor length invalid. cdTextDescLen:%d", cdTextDescSize);
        free(cdTextDat);
        return ESP_FAIL;
    }

    // 获取所有字符串长度
    // get total string length
    uint16_t titleStrBufSize = 0;
    uint16_t performerStrBufSize = 0;
    for (int i = 0; i < cdTextDescSize / 18; i++)
    {
        if ((textSequence + i)->ID1 == 0x80)
            titleStrBufSize += 12;
        if ((textSequence + i)->ID1 == 0x81)
            performerStrBufSize += 12;
    }

    // 申请字符串内存
    // allocate string memory
    *titleStrBuf = (char *)malloc(titleStrBufSize);
    *performerStrBuf = (char *)malloc(performerStrBufSize);
    if (*titleStrBuf == NULL || *performerStrBuf == NULL)
    {
        ESP_LOGE("cpplayer_getCdText", "String buffer malloc fail");
        if (*titleStrBuf != NULL)
            free(*titleStrBuf);
        if (*performerStrBuf != NULL)
            free(*performerStrBuf);
        free(cdTextDat);
        return ESP_FAIL;
    }
    memset(*titleStrBuf, 0, titleStrBufSize);
    memset(*performerStrBuf, 0, performerStrBufSize);

    // 遍历每一个sequence
    // foreach all text sequence
    int titleStrFoundCount = 0;
    int titleStrInsertAt = 0;
    int performerStrFoundCount = 0;
    int performerStrInsertAt = 0;
    for (int i = 0; i < cdTextDescSize / 18; i++)
    {
        if (textSequence->ID1 == 0x80) // title text
        {
            uint8_t trackNum = textSequence->ID2;

            if (trackNum != titleStrFoundCount)
            {
                ESP_LOGE("cpplayer_getCdText", "Track number does not match the number of discovered titles strings.");
                printMem(cdTextDat, cdTextDescSize);
                free(*titleStrBuf);
                free(*performerStrBuf);
                free(cdTextDat);
                return ESP_FAIL;
            }

            if ((textSequence->ID4 & 0x0f) == 0)
            {
                if (trackNum == 0)
                    *albumTitle = *titleStrBuf + titleStrInsertAt;
                else
                    trackList[trackNum - 1].title = *titleStrBuf + titleStrInsertAt;
            }

            memcpy(*titleStrBuf + titleStrInsertAt, textSequence->text, 12);

            for (int charI = 0; charI < 12; charI++)
            {
                if (textSequence->text[charI] == '\0' && trackNum <= tracksCount)
                {
                    trackList[trackNum].title = *titleStrBuf + titleStrInsertAt + charI + 1;
                    trackNum++;
                }
            }

            titleStrFoundCount = trackNum;
            titleStrInsertAt += 12;
        }
        else if (textSequence->ID1 == 0x81) // performer text
        {
            uint8_t trackNum = textSequence->ID2;

            if (trackNum != performerStrFoundCount)
            {
                ESP_LOGE("cpplayer_getCdText", "Track number does not match the number of discovered performers strings.");
                printMem(cdTextDat, cdTextDescSize);
                free(*titleStrBuf);
                free(*performerStrBuf);
                free(cdTextDat);
                return ESP_FAIL;
            }

            if ((textSequence->ID4 & 0x0f) == 0)
            {
                if (trackNum == 0)
                    *albumPerformer = *performerStrBuf + performerStrInsertAt;
                else
                    trackList[trackNum - 1].performer = *performerStrBuf + performerStrInsertAt;
            }

            memcpy(*performerStrBuf + performerStrInsertAt, textSequence->text, 12);

            for (int charI = 0; charI < 12; charI++)
            {
                if (textSequence->text[charI] == '\0' && trackNum <= tracksCount)
                {
                    trackList[trackNum].performer = *performerStrBuf + performerStrInsertAt + charI + 1;
                    trackNum++;
                }
            }

            performerStrFoundCount = trackNum;
            performerStrInsertAt += 12;
        }

        textSequence++;
    }

    free(cdTextDat);
    return ESP_OK;
}

void volumeStep(int upDown)
{
    cdplayer_playerInfo.volume += upDown;
    if (cdplayer_playerInfo.volume > 30)
        cdplayer_playerInfo.volume = 30;
    if (cdplayer_playerInfo.volume < 0)
        cdplayer_playerInfo.volume = 0;

    ESP_LOGI("volumeStep", "Volume: %d", cdplayer_playerInfo.volume);
}

void cdplayer_task_deviceAndDiscMonitor(void *arg)
{
    esp_err_t err;
    uint8_t responDat[200] = {0};
    uint32_t responSize;

    while (1)
    {
        cdplayer_driveInfo.discInserted = 0;
        cdplayer_driveInfo.discIsCD = 0;
        cdplayer_driveInfo.trayClosed = 1;
        cdplayer_driveInfo.trackCount = 0;
        cdplayer_driveInfo.cdTextAvalibale = 0;
        cdplayer_driveInfo.readyToPlay = 0;
        cdplayer_driveInfo.albumTitle = NULL;
        cdplayer_driveInfo.albumPerformer = NULL;
        cdplayer_driveInfo.strBuf_titles = NULL;
        cdplayer_driveInfo.strBuf_performers = NULL;

        cdplayer_playerInfo.playing = 0;
        cdplayer_playerInfo.playingTrackIndex = 0;
        cdplayer_playerInfo.readFrameCount = 0;

        strcpy(cdplayer_driveInfo.vendor, "");
        strcpy(cdplayer_driveInfo.product, "");

        // 等待设备连接
        // wait for usb device connect
        while (usbhost_driverObj.deviceIsOpened == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            printf("Wait for usb cd driver connect.\n");
        }

        // 检查是不是光驱
        // Check if usb device is CD/DVD device
        ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Check if usb device is CD/DVD device.");
        responSize = 32;
        err = usbhost_scsi_inquiry(responDat, &responSize);
        if (err != ESP_OK)
        {
            ESP_LOGE("cdplayer_task_deviceAndDiscMonitor", "SCSI inquiry cmd fail: %d", err);
            continue;
        }
        if ((responDat[0] & 0x0f) != 0x05) // SPC-3 Table 83 — Peripheral device type
        {
            printf("Not CD/DVD device.\n");
            usbhost_closeDevice();
            continue;
        }
        if (responSize == 32) // get vendor id and product id
        {
            memcpy(cdplayer_driveInfo.vendor, (responDat + 8), 8);
            cdplayer_driveInfo.vendor[8] = '\0';
            memcpy(cdplayer_driveInfo.product, (responDat + 16), 16);
            cdplayer_driveInfo.product[16] = '\0';
            int i;
            for (i = 7; i >= 0; i--)
            {
                if (cdplayer_driveInfo.vendor[i] != ' ')
                {
                    cdplayer_driveInfo.vendor[i + 1] = '\0';
                    break;
                }
            }
            for (i = 15; i >= 0; i--)
            {
                if (cdplayer_driveInfo.product[i] != ' ')
                {
                    cdplayer_driveInfo.product[i + 1] = '\0';
                    break;
                }
            }
            printf("Device model: %s %s\n", cdplayer_driveInfo.vendor, cdplayer_driveInfo.product);
        }

        // 等待光盘插入
        // Wait for disc insert
        ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Wait for disc insert");
        while (1)
        {
            err = cdplayer_discReady(&cdplayer_driveInfo.discInserted,
                                     &cdplayer_driveInfo.trayClosed);
            if (err == ESP_OK || !usbhost_driverObj.deviceIsOpened)
                break;
            printf("Unit not ready, tray: %s\n", cdplayer_driveInfo.trayClosed ? "Closed" : "Open");
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        if (!usbhost_driverObj.deviceIsOpened)
            continue;

        // 检查碟是否为cd
        // Check if disc is cdda
        ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Check if disc is cdda");
        cdplayer_driveInfo.discIsCD = cdplayer_discIsCd();
        if (!cdplayer_driveInfo.discIsCD)
        {
            ESP_LOGE("cdplayer_task_deviceAndDiscMonitor", "Not CD-DA");
            goto WAIT_FOR_DISC_REMOVE;
        }

        // 读toc
        // Read TOC
        ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Read TOC");
        err = cdplayer_getPlayList(&cdplayer_driveInfo.trackCount, cdplayer_driveInfo.trackList);
        if (err != ESP_OK)
            continue;
        if (cdplayer_driveInfo.trackCount == 0)
        {
            ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Not CD-DA");
            cdplayer_driveInfo.discIsCD = 0;
            goto WAIT_FOR_DISC_REMOVE;
        }

        // 读cd-text(如果有)，从这里开始要注意free了
        // Read CD-TEXT
        ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Read CD-TEXT");
        cdplayer_driveInfo.cdTextAvalibale = false;
        err = cpplayer_getCdText(
            &cdplayer_driveInfo.albumTitle, &cdplayer_driveInfo.albumPerformer,
            &cdplayer_driveInfo.strBuf_titles, &cdplayer_driveInfo.strBuf_performers,
            cdplayer_driveInfo.trackCount, cdplayer_driveInfo.trackList);
        if (err == ESP_OK)
        {
            cdplayer_driveInfo.cdTextAvalibale = true;
            printf("Album performent: %s\nAlbum title: %s\n",
                   cdplayer_driveInfo.albumPerformer, cdplayer_driveInfo.albumTitle);
        }
        else
        {
            ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "CD-TEXT not found");
            if (cdplayer_driveInfo.strBuf_titles != NULL)
                free(cdplayer_driveInfo.strBuf_titles);
            if (cdplayer_driveInfo.strBuf_performers != NULL)
                free(cdplayer_driveInfo.strBuf_performers);
        }

        // 打印播放列表
        // print play list
        printf("**********PlayList**********\n");
        for (int i = 0; i < cdplayer_driveInfo.trackCount; i++)
        {
            printf("Track: %02d, begin: %6ld, duration: %6ld, preEmphasis: %d ",
                   cdplayer_driveInfo.trackList[i].trackNum,
                   cdplayer_driveInfo.trackList[i].lbaBegin,
                   cdplayer_driveInfo.trackList[i].trackDuration,
                   cdplayer_driveInfo.trackList[i].preEmphasis);
            if (cdplayer_driveInfo.cdTextAvalibale)
                printf("%s - %s\n",
                       cdplayer_driveInfo.trackList[i].performer,
                       cdplayer_driveInfo.trackList[i].title);
            else
                printf("\n");
        }

        cdplayer_driveInfo.readyToPlay = 1;
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 等待碟片弹出或光驱移除
        // wait for disc remove
    WAIT_FOR_DISC_REMOVE:
        while (1)
        {
            for (int i = 0; i < 7; i++)
            {
                err = cdplayer_discReady(&cdplayer_driveInfo.discInserted,
                                         &cdplayer_driveInfo.trayClosed);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            if (err != ESP_OK)
            {
                ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "Disc removed");
                break;
            }
            if (usbhost_driverObj.deviceIsOpened != 1)
            {
                ESP_LOGI("cdplayer_task_deviceAndDiscMonitor", "CD driver disconnected");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        // 碟片移除
        // free cd text string memory
        if (cdplayer_driveInfo.cdTextAvalibale)
        {
            free(cdplayer_driveInfo.strBuf_titles);
            free(cdplayer_driveInfo.strBuf_performers);
        }
    }
}

void cdplayer_task_playControl(void *arg)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        btn_renew(0);

        // 弹出碟片
        // eject disc
        if (btn_getPosedge(BTN_EJECT))
        {
            ESP_LOGI("cdplayer_task_playControl", "Eject disc");
            esp_err_t err = usbhost_scsi_startStopUnit(true, false);
            if (err != ESP_OK)
            {
                uint8_t senseDat[18];
                usbhost_scsi_requestSense(senseDat);
                uint8_t senseKey = senseDat[2] & 0x0f;
                uint8_t asc = senseDat[12];
                uint8_t ascq = senseDat[13];
                printf("Eject fail, sense key:%02x, asc:%02x%02x\n", senseKey, asc, ascq);
            }
        }

        // 修改音量
        // change volume
        static bool volumHasChange = false;
        if (btn_getNegedge(BTN_VOL_UP))
        {
            volumHasChange = true;
            volumeStep(1);
        }
        if (btn_getNegedge(BTN_VOL_DOWN))
        {
            volumHasChange = true;
            volumeStep(-1);
        }
        static struct timeval t;
        struct timeval now;
        if (btn_getLongPress(BTN_VOL_UP, 0))
        {
            gettimeofday(&now, NULL);
            uint32_t durationMs = (now.tv_sec - t.tv_sec) * 1000 + (now.tv_usec - t.tv_usec) / 1000;
            if (durationMs > 50)
            {
                gettimeofday(&t, NULL);
                volumeStep(1);
                volumHasChange = true;
            }
        }
        if (btn_getLongPress(BTN_VOL_DOWN, 0))
        {
            gettimeofday(&now, NULL);
            uint32_t durationMs = (now.tv_sec - t.tv_sec) * 1000 + (now.tv_usec - t.tv_usec) / 1000;
            if (durationMs > 50)
            {
                gettimeofday(&t, NULL);
                volumeStep(-1);
                volumHasChange = true;
            }
        }

        // 保存音量
        // save volume
        if (volumHasChange && !cdplayer_playerInfo.playing)
        {
            volumHasChange = false;
            int8_t savedValue;
            nvs_handle_t my_handle;
            nvs_open("storage", NVS_READWRITE, &my_handle);
            nvs_get_i8(my_handle, "vol", &savedValue);
            if (savedValue != cdplayer_playerInfo.volume)
            {
                nvs_set_i8(my_handle, "vol", cdplayer_playerInfo.volume);
                nvs_commit(my_handle);
            }
            nvs_close(my_handle);
            ESP_LOGI("cdplayer_task_playControl", "volum saved.");
        }

        // 快进快退
        // fast forward, fast backward
        if (btn_getLongPress(BTN_NEXT, 0))
        {
            cdplayer_playerInfo.fastForwarding = 1;
            cdplayer_playerInfo.readFrameCount += 5;
            if (cdplayer_playerInfo.readFrameCount > cdplayer_driveInfo.trackList[cdplayer_playerInfo.playingTrackIndex].trackDuration)
            {
                cdplayer_playerInfo.readFrameCount = cdplayer_driveInfo.trackList[cdplayer_playerInfo.playingTrackIndex].trackDuration;
            }
        }
        else if (btn_getLongPress(BTN_PREVIOUS, 0))
        {
            cdplayer_playerInfo.fastBackwarding = 1;
            cdplayer_playerInfo.readFrameCount -= 5;
            if (cdplayer_playerInfo.readFrameCount <= 0)
            {
                cdplayer_playerInfo.readFrameCount = 0;
            }
        }

        // 下一曲 上一曲
        // Next/Previous song
        if (btn_getPosedge(BTN_NEXT))
        {
            if (cdplayer_playerInfo.fastForwarding)
            {
                cdplayer_playerInfo.fastForwarding = 0;
            }
            else
            {
                cdplayer_playerInfo.readFrameCount = 0;
                cdplayer_playerInfo.playingTrackIndex++;
                if (cdplayer_playerInfo.playingTrackIndex >= cdplayer_driveInfo.trackCount)
                    cdplayer_playerInfo.playingTrackIndex = 0;
                ESP_LOGI("cdplayer_task_playControl", "Next, play: %d", cdplayer_playerInfo.playingTrackIndex);
            }
        }
        else if (btn_getPosedge(BTN_PREVIOUS))
        {
            if (cdplayer_playerInfo.fastBackwarding)
            {
                cdplayer_playerInfo.fastBackwarding = 0;
            }
            else
            {
                cdplayer_playerInfo.readFrameCount = 0;
                cdplayer_playerInfo.playingTrackIndex--;
                if (cdplayer_playerInfo.playingTrackIndex < 0)
                    cdplayer_playerInfo.playingTrackIndex = cdplayer_driveInfo.trackCount - 1;
                ESP_LOGI("cdplayer_task_playControl", "Pervious, play: %d", cdplayer_playerInfo.playingTrackIndex);
            }
        }

        // 播放暂停
        // play pause
        if (btn_getPosedge(BTN_PLAY))
        {
            if (cdplayer_driveInfo.readyToPlay == 1)
            {
                cdplayer_playerInfo.playing = !cdplayer_playerInfo.playing;
                ESP_LOGI("cdplayer_task_playControl", "Play: %d", cdplayer_playerInfo.playing);
                if (cdplayer_playerInfo.playing)
                {
                    esp_err_t err = usbhost_scsi_setCDSpeed(65535);
                    if (err != ESP_OK)
                    {
                        uint8_t senseDat[18];
                        usbhost_scsi_requestSense(senseDat);
                        uint8_t senseKey = senseDat[2] & 0x0f;
                        uint8_t asc = senseDat[12];
                        uint8_t ascq = senseDat[13];
                        printf("Set cd speed fail, sense key:%02x, asc:%02x%02x\n", senseKey, asc, ascq);
                    }
                }
            }
        }

        // 读cd
        // read cd and sent to i2s buffer
        if (cdplayer_driveInfo.readyToPlay == 1 && cdplayer_playerInfo.playing &&
            !cdplayer_playerInfo.fastForwarding && !cdplayer_playerInfo.fastBackwarding &&
            !i2s_bufsFull)
        {
            int8_t *trackNo = &cdplayer_playerInfo.playingTrackIndex;
            uint32_t trackDuration = cdplayer_driveInfo.trackList[*trackNo].trackDuration;
            int32_t *readFrameCount = &cdplayer_playerInfo.readFrameCount;
            uint32_t remainFrame = trackDuration - *readFrameCount;

            uint32_t readFrames = (remainFrame > I2S_TX_BUFFER_SIZE_FRAME) ? I2S_TX_BUFFER_SIZE_FRAME : remainFrame;
            uint32_t readBytes = readFrames * 2352;
            uint32_t readLba = cdplayer_driveInfo.trackList[*trackNo].lbaBegin + *readFrameCount;

            printf("Read lba:%ld, len: %ld\n", readLba, readFrames);
            esp_err_t err = usbhost_scsi_readCD(readLba, readCdBuf, &readFrames, &readBytes);

            if (err == ESP_OK)
            {
                i2s_fillBuffer(readCdBuf);

                *readFrameCount += readFrames;
            }
            else
            {
                printf("Read fail, lba: %ld len: %ld\n", readLba, readBytes);

                uint8_t senseDat[18];
                usbhost_scsi_requestSense(senseDat);
                uint8_t senseKey = senseDat[2] & 0x0f;
                uint8_t asc = senseDat[12];
                uint8_t ascq = senseDat[13];
                printf("sense key: %02x, asc: %02x%02x\n", senseKey, asc, ascq);
            }

            // 播放完成判断
            // track finish playing
            if (*readFrameCount >= trackDuration)
            {
                *readFrameCount = 0;
                (*trackNo)++;
                if (*trackNo >= cdplayer_driveInfo.trackCount)
                {
                    cdplayer_playerInfo.playing = 0;
                    *trackNo = 0;
                    ESP_LOGI("cdplayer_task_playControl", "Finish");
                }
                else
                {
                    ESP_LOGI("cdplayer_task_playControl", "Play next track: %02d", (*trackNo) + 1);
                }
            }
        }
    }
}

void cdplay_init()
{
    // 读音量
    // load volume
    nvs_handle_t my_handle;
    esp_err_t err;
    nvs_open("storage", NVS_READWRITE, &my_handle);
    err = nvs_get_i8(my_handle, "vol", &cdplayer_playerInfo.volume);
    if (err != ESP_OK)
        cdplayer_playerInfo.volume = 10;
    nvs_close(my_handle);

    BaseType_t ret;
    ret = xTaskCreatePinnedToCore(cdplayer_task_deviceAndDiscMonitor,
                                  "cdplayer_task_deviceAndDiscMonitor",
                                  4096,
                                  NULL,
                                  2,
                                  NULL,
                                  0);
    if (ret != pdPASS)
        ESP_LOGE("cdplay_init", "cdplayer_task_deviceAndDiscMonitor creat fail");

    ret = xTaskCreatePinnedToCore(cdplayer_task_playControl,
                                  "cdplayer_task_playControl",
                                  4096,
                                  NULL,
                                  3,
                                  NULL,
                                  0);
    if (ret != pdPASS)
        ESP_LOGE("cdplay_init", "cdplayer_task_playControl creat fail");
}

hmsf_t cdplay_frameToHmsf(uint32_t frame)
{
    int sec = frame / 75;

    hmsf_t result = {
        .hour = sec / 3600,
        .minute = sec % 3600 / 60,
        .second = sec % 60,
        .frame = frame % 75,
    };
    return result;
}