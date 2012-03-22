#ifndef USBCORE_H
#define USBCORE_H

#include "usb/usbdefs.h"
#include "usb/usbd.h"

class UsbCore
{
public:
    UsbCore();

    static uint16_t buildConfigDescriptor(uint8_t index, uint8_t *buf, uint16_t len);
    static int getDescriptor(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static int setAddress(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static int setConfiguration(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int getConfiguration(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static int setInterface(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int getInterface(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
};

#endif // USBCORE_H
