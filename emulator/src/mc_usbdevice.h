#ifndef MC_USBDEVICE_H
#define MC_USBDEVICE_H

#include <stdint.h>

class UsbDevice
{
public:
    UsbDevice();    // not implemented

    static void handleOUTData();
    static int write(const uint8_t *buf, unsigned len, unsigned timeoutMillis = 0xffffffff);
};

#endif // MC_USBDEVICE_H
