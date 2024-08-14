/**
 *
 * Mass Storage Class
 * Bulk-Only Transport class-specific commands
 * 类特殊请求
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

#include "usbhost_msc_cmd.h"

uint32_t dCBWTag = 0x01145140; // 哼
SemaphoreHandle_t scsiExeLock;

esp_err_t usbhost_resetRecovery()
{
    // USB Mass Storage Class – Bulk Only Transport Revision 1.0
    // 5.3.3.1 Phase Error
    // The host shall perform a Reset Recovery when Phase Error status is returned in the CSW.
    // 5.3.4 Reset Recovery
    // For Reset Recovery the host shall issue in the following order: :
    // (a) a Bulk-Only Mass Storage Reset
    // (b) a Clear Feature HALT to the Bulk-In endpoint
    // (c) a Clear Feature HALT to the Bulk-Out endpoint
    ESP_LOGI("usbhost_resetRecovery", "Bulk-Only Mass Storage Reset");
    usbhost_cmd_bulkOnlyMassStorageReset();
    ESP_LOGI("usbhost_resetRecovery", "clearFeature ep_in_num");
    usbhost_clearFeature(usbhost_driverObj.ep_in_num);
    ESP_LOGI("usbhost_resetRecovery", "clearFeature ep_out_num");
    usbhost_clearFeature(usbhost_driverObj.ep_out_num);

    return ESP_OK;
}

// Bulk-Only Mass Storage Reset
esp_err_t usbhost_cmd_bulkOnlyMassStorageReset()
{
    usb_setup_packet_t setupPack = {
        .bmRequestType = 0x21, // out pack, to class interface
        .bRequest = 0xff,      // Bulk-Only Mass Storage Reset
        .wValue = 0,
        .wIndex = usbhost_driverObj.desc_interface->bInterfaceNumber,
        .wLength = 0,
    };

    usbhost_controlTransfer(&setupPack, 8);

    return ESP_OK;
}

// Get Max LUN
esp_err_t usbhost_cmd_getMaxLun(uint8_t *lun)
{
    uint8_t data[9];
    usb_setup_packet_t *setupPack = (usb_setup_packet_t *)data;
    setupPack->bmRequestType = 0xa1; // in pack, from class interface
    setupPack->bRequest = 0xfe;      // Get Max LUN
    setupPack->wValue = 0;
    setupPack->wIndex = usbhost_driverObj.desc_interface->bInterfaceNumber;
    setupPack->wLength = 1; // read 1 byte

    usbhost_controlTransfer(data, 9);

    *lun = data[8];

    return ESP_OK;
}

esp_err_t usbhost_cmd_cbwExecute(void *cbwcb, uint8_t cbwcbLen, void *data, uint32_t *dataLen, usbhost_transDir_t dataDir, uint32_t timeout)
{

    uint8_t cbwBuf[31];
    memset(cbwBuf, 0, 31);

    uint32_t cbwLen = 31;

    usbhost_msc_cbw_t *cbw = (usbhost_msc_cbw_t *)cbwBuf;
    cbw->dCBWSignature = 0x43425355; //"USBC"
    cbw->dCBWTag = dCBWTag++;
    cbw->dCBWDataTransferLength = *dataLen;
    cbw->bmCBWFlags = (dataDir == HOST_TO_DEV) ? 0x00 : 0x80;
    cbw->bCBWLUN = 0; // Not considering multiple LUNs devices
    cbw->bCBWCBLength = cbwcbLen & 0x1f;
    memcpy(&cbwBuf[15], cbwcb, cbwcbLen);

    esp_err_t err;

    // 1. Command transport
    err = usbhost_bulkTransfer(&cbwBuf, &cbwLen, HOST_TO_DEV, 200);
    if (err != ESP_OK)
    {
        return err;
    }

    // 2. Optional data transport
    if (*dataLen > 0)
    {
        uint32_t transferDatLen = *dataLen;
        uint8_t epAddr = (dataDir == HOST_TO_DEV) ? usbhost_driverObj.ep_out_num : usbhost_driverObj.ep_in_num;

        err = usbhost_bulkTransfer(data, &transferDatLen, dataDir, timeout);
        if (err == USB_TRANSFER_STATUS_STALL)
        {
            ESP_LOGE("usbhost_cmd_cbwExecute", "ep stall, cbw command fail");
            usbhost_clearFeature(epAddr);
            return err;
        }
        else if (transferDatLen < *dataLen)
        {
            usbhost_clearFeature(epAddr);
            *dataLen = transferDatLen;
        }
    }

    // 3. Status transport
    usbhost_msc_csw_t csw;
    uint32_t cswLen = sizeof(usbhost_msc_csw_t);
    if (*dataLen > 0)
        err = usbhost_bulkTransfer(&csw, &cswLen, DEV_TO_HOST, 200);
    else
        err = usbhost_bulkTransfer(&csw, &cswLen, DEV_TO_HOST, timeout);

    // 3.1 Error recovery
    if (err == USB_TRANSFER_STATUS_STALL)
    {
        ESP_LOGI("usbhost_cmd_cbwExecute", "read csw fail, clear feature and try again");
        // clear endpoint
        usbhost_clearFeature(usbhost_driverObj.ep_in_num);

        // read again
        cswLen = sizeof(usbhost_msc_csw_t);
        err = usbhost_bulkTransfer(&csw, &cswLen, DEV_TO_HOST, 200);
        if (err != ESP_OK)
        {
            ESP_LOGI("usbhost_cmd_cbwExecute", "read csw fail again, reset recovery");
            usbhost_resetRecovery();
            return err;
        }
    }

    // check csw
    bool signatureOK = false;
    bool tagOK = false;
    bool stateOK = false;
    if (csw.dCSWSignature == 0x53425355) // "USBS"
        signatureOK = true;
    if (csw.dCSWTag == cbw->dCBWTag)
        tagOK = true;
    if (csw.bCSWStatus == 0)
        stateOK = true;

    if (signatureOK & tagOK & stateOK)
        return USB_TRANSFER_STATUS_COMPLETED;
    else
        return ESP_FAIL;
}