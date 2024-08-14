#ifndef __USBHOST_SCSI_CMD_H_
#define __USBHOST_SCSI_CMD_H_

#include "usb/usb_host.h"
#include "usbhost_msc_cmd.h"

// MMC-4 Table 437 - READ TOC/PMA/ATIP Data list, general definition
// big-endian
typedef struct __attribute__((packed))
{
    uint16_t TOC_Data_Length;
    uint8_t First_Track_Number;
    uint8_t Last_Track_Number;
} usbhost_scsi_tocHeader_t;
// MMC-4 Table 438 - READ TOC/PMA/ATIP response data (Format = 0000b)
typedef struct __attribute__((packed))
{
    uint8_t Reserved;
    uint8_t ADR_CONTROL;
    uint8_t Track_Number;
    uint8_t Reserved2;
    uint32_t Track_Start_Address;
} usbhost_scsi_tocTrackDesriptor_t;
// IEC 60908-1999 26.2.2 CD TEXT PACK format for the lead-in area
typedef struct __attribute__((packed))
{
    uint8_t ID1;
    uint8_t ID2;
    uint8_t ID3;
    uint8_t ID4;
    char text[12];
    uint16_t crc;
} usbhost_scsi_tocCdTextDesriptor_t;

extern SemaphoreHandle_t scsiExeLock;

esp_err_t usbhost_scsi_inquiry(uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_requestSense(uint8_t *responData);
esp_err_t usbhost_scsi_getEventStatusNotification(uint8_t requestClass, uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_getConfiguration(uint16_t featureNum, uint8_t rt, uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_modeSense10(uint8_t pageCode, uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_reportKey(uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_testUnitReady();
esp_err_t usbhost_scsi_readCapacity(uint32_t *logicalBlockAddress, uint32_t *blockLengthInBytes);
esp_err_t usbhost_scsi_startStopUnit(bool LoEj, bool Start);
esp_err_t usbhost_scsi_readTOC(bool time, uint8_t format, uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_readDiscInformation(uint8_t *responData, uint32_t *len);
esp_err_t usbhost_scsi_readCD(uint32_t lba, uint8_t *responData, uint32_t *transFrame, uint32_t *readSize);
esp_err_t usbhost_scsi_setCDSpeed(uint16_t readSpeed);

#endif
