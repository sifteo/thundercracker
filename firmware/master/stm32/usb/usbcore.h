#ifndef USBCORE_H
#define USBCORE_H

#include "usb/usbdefs.h"
#include "usb/usbd.h"

class UsbCore
{
public:
    UsbCore();

    static int getDescriptor(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static int setAddress(Usb::SetupData *req);

    static int setConfiguration(Usb::SetupData *req);
    static int getConfiguration(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static int setInterface(Usb::SetupData *req, uint16_t *len);
    static int getInterface(uint8_t **buf, uint16_t *len);

    static int standardDeviceRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardInterfaceRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardEndpointRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

private:
    static uint16_t buildConfigDescriptor(uint8_t index, uint8_t *buf, uint16_t len);

    static int getDeviceStatus(uint8_t **buf, uint16_t *len);
    static int getInterfaceStatus(uint8_t **buf, uint16_t *len);
    static int getEndpointStatus(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
};

#endif // USBCORE_H
