#ifndef __USBHOST_DRIVER_H_
#define __USBHOST_DRIVER_H_

#include "usb/usb_host.h"

typedef enum
{
    HOST_TO_DEV,
    DEV_TO_HOST
} usbhost_transDir_t;

typedef struct
{
    usb_host_client_handle_t handle_client;
    usb_device_handle_t handle_device;
    uint8_t dev_addr;
    uint8_t deviceIsOpened;

    usb_intf_desc_t *desc_interface;
    usb_ep_desc_t *desc_ep_out;
    usb_ep_desc_t *desc_ep_in;

    uint8_t ep_out_num;
    uint16_t ep_out_packsize;
    uint8_t ep_in_num;
    uint16_t ep_in_packsize;

    usb_transfer_t *transferObj;
    SemaphoreHandle_t transferDone;

} usbhost_driver_t;

extern usbhost_driver_t usbhost_driverObj;

void usbhost_driverInit();
esp_err_t usbhost_openDevice();
void usbhost_closeDevice();

esp_err_t usbhost_clearFeature(uint8_t endpoint);
esp_err_t usbhost_controlTransfer(void *data, size_t size);
esp_err_t usbhost_bulkTransfer(void *data, uint32_t *size, usbhost_transDir_t dir, uint32_t timeoutMs);

#endif