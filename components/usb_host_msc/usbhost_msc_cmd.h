#ifndef __USBHOST_MSC_CMD_H_
#define __USBHOST_MSC_CMD_H_

#include "usb/usb_host.h"
#include "usbhost_driver.h"

// Command Block Wrapper structure
// USB Mass Storage Class – Bulk Only Transport, Table 5.1
typedef struct __attribute__((packed))
{
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
} usbhost_msc_cbw_t;

// Command Status Wrapper structure
// USB Mass Storage Class – Bulk Only Transport, Table 5.2
typedef struct __attribute__((packed))
{
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} usbhost_msc_csw_t;


esp_err_t usbhost_cmd_bulkOnlyMassStorageReset();
esp_err_t usbhost_cmd_getMaxLun(uint8_t *lun);
esp_err_t usbhost_cmd_cbwExecute(void *cbwcb, uint8_t cbwcbLen, void *data, uint32_t *dataLen, usbhost_transDir_t dataDir, uint32_t timeout);

#endif