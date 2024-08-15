/**
 *
 * Some SCSI Commands for CD/DVD device
 *
 * Refer to:
 *    Universal Serial Bus Mass Storage Class UFI Command Specification Revision 1.0
 *    SCSI Primary Commands-3 (SPC-3)
 *    SCSI Block Commands - 3 (SBC-3)
 *    SCSI Multimedia Commands - 4 (MMC-4)
 *    SCSI Multi-Media Commands – 6 (MMC-6)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "usbhost_scsi_cmd.h"

// UFI 4.2 INQUIRY Command: 12h
esp_err_t usbhost_scsi_inquiry(uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[6];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x12;                    // Operation Code (12h)
    cbwcb[3] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[4] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// SPC-3 6.27 REQUEST SENSE command
// respon size 18 bytes
esp_err_t usbhost_scsi_requestSense(uint8_t *responData)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint32_t requireLen = 18;

    uint8_t cbwcb[12];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x03;       // OPERATION CODE (03h)
    cbwcb[4] = requireLen; // ALLOCATION LENGTH

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, &requireLen, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.7 GET EVENT STATUS NOTIFICATION Command
esp_err_t usbhost_scsi_getEventStatusNotification(uint8_t requestClass, uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[10];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x4a;                    // OPERATION CODE (4Ah)
    cbwcb[1] = 0x01;                    // Polled
    cbwcb[4] = requestClass & 0x7e;     // Notification Class Request
    cbwcb[7] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[8] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.6 GET CONFIGURATION Command
esp_err_t usbhost_scsi_getConfiguration(uint16_t featureNum, uint8_t rt, uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[10];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x46;                    // OPERATION CODE (46h)
    cbwcb[1] = rt & 0x03;               // RT
    cbwcb[2] = featureNum >> 8;         // Starting Feature Number(MSB)
    cbwcb[3] = featureNum & 0xff;       // Starting Feature Number(LSB)
    cbwcb[7] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[8] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// SPC-3 6.10 MODE SENSE(10) command
// MMC-4 7.1.3 Mode Pages
esp_err_t usbhost_scsi_modeSense10(uint8_t pageCode, uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[10];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x5a;                    // OPERATION CODE (5Ah)
    cbwcb[1] = 0x08;                    // disable block descriptors
    cbwcb[2] = pageCode & 0x3f;         // PC[7:6]: Current values, PAGE CODE[5:0]， 2Ah: MM Capabilities & Mechanical Status mode page
    cbwcb[7] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[8] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.33 REPORT KEY Command
esp_err_t usbhost_scsi_reportKey(uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[12];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0xa4;                    // Operation Code (A4h)
    cbwcb[8] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[9] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)
    cbwcb[10] = 0x08;                   // KEY Format: RPC State

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// SPC-3 6.33 TEST UNIT READY command
esp_err_t usbhost_scsi_testUnitReady()
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[6];
    memset(cbwcb, 0, sizeof(cbwcb));

    uint32_t requireLen = 0;

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), NULL, &requireLen, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.23 READ CAPACITY Command
esp_err_t usbhost_scsi_readCapacity(uint32_t *logicalBlockAddress, uint32_t *blockLengthInBytes)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[10];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x25; // OPERATION CODE (25h)

    uint8_t requireDat[8];
    uint32_t requireLen = 8;

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), requireDat, &requireLen, DEV_TO_HOST, 500);
    xSemaphoreGive(scsiExeLock);

    if (err == ESP_OK)
    {
        *logicalBlockAddress = __builtin_bswap32(*(uint32_t *)(requireDat));
        *blockLengthInBytes = __builtin_bswap32(*(uint32_t *)(requireDat + 4));
        return ESP_OK;
    }
    else
    {
        return err;
    }
}

// MMC-4 6.45 START STOP UNIT Command
esp_err_t usbhost_scsi_startStopUnit(bool LoEj, bool Start)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[12];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x1b; // Operation Code (1Bh)
    if (LoEj)
        cbwcb[4] |= 0x02;
    if (Start)
        cbwcb[4] |= 0x01;

    uint32_t requireLen = 0;

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), NULL, &requireLen, DEV_TO_HOST, 5678);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.30 READ TOC/PMA/ATIP Command
esp_err_t usbhost_scsi_readTOC(bool time, uint8_t format, uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[10];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x43;     // OPERATION CODE (43h)
    if (time)            // 0:LBA form, 1: TIME form
        cbwcb[1] = 0x02; // TIME form = HMSF
    cbwcb[2] = format & 0x0f;
    cbwcb[6] = 0;                       // start from track 1
    cbwcb[7] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[8] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 10000);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.26 READ DISC INFORMATION Command
esp_err_t usbhost_scsi_readDiscInformation(uint8_t *responData, uint32_t *len)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[10];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0x51;                    // Operation Code (51h)
    cbwcb[7] = *((uint8_t *)(len) + 1); // ALLOCATION LENGTH (MSB)
    cbwcb[8] = *((uint8_t *)(len) + 0); // ALLOCATION LENGTH (LSB)

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, len, DEV_TO_HOST, 5000);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.24 READ CD Command
esp_err_t usbhost_scsi_readCD(uint32_t lba, uint8_t *responData, uint32_t *transFrame, uint32_t *readSize)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[12];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0xbe;                                     // Operation Code (BEh)
    cbwcb[1] = 0x00;                                     // read all type, no modified by flaw obscuring mechanisms
    *((uint32_t *)(cbwcb + 2)) = __builtin_bswap32(lba); // Starting Logical Block Address
    cbwcb[6] = *((uint8_t *)(transFrame) + 2);           // Transfer Length (MSB)
    cbwcb[7] = *((uint8_t *)(transFrame) + 1);           // Transfer Length
    cbwcb[8] = *((uint8_t *)(transFrame) + 0);           // Transfer Length (LSB)
    cbwcb[9] = 0x10;                                     // Main Channel Selection Bits: User Data, no C2 Error Information
                                                         // for cdda 10h and f8h seems like the same

    *readSize = 2352 * (*transFrame);

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), responData, readSize, DEV_TO_HOST, 10000);

    xSemaphoreGive(scsiExeLock);
    return err;
}

// MMC-4 6.42 SET CD SPEED Command
esp_err_t usbhost_scsi_setCDSpeed(uint16_t readSpeed)
{
    xSemaphoreTake(scsiExeLock, portMAX_DELAY);

    uint8_t cbwcb[12];
    memset(cbwcb, 0, sizeof(cbwcb));

    cbwcb[0] = 0xbb;                                           // Operation Code (BBh)
    cbwcb[1] = 0x00;                                           // Rotational Control: CLV and non-pure CAV
    *((uint16_t *)(cbwcb + 2)) = __builtin_bswap16(readSpeed); // Logical Unit Read Speed (bytes/sec)
    *((uint16_t *)(cbwcb + 4)) = __builtin_bswap16(0);         // Logical Unit Write Speed (bytes/sec)

    uint32_t requireLen = 0;

    esp_err_t err = usbhost_cmd_cbwExecute(cbwcb, sizeof(cbwcb), NULL, &requireLen, DEV_TO_HOST, 500);

    xSemaphoreGive(scsiExeLock);
    return err;
}