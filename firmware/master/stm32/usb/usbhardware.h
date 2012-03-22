#ifndef USB_HARDWARE_H
#define USB_HARDWARE_H

#include "usb/usbdefs.h"

/*
 * Platform specific interface to perform hardware related USB tasks.
 */
class UsbHardware
{
public:
    UsbHardware();

    static void init();
    static void setAddress(uint8_t addr);
    static void epSetup(uint8_t addr, uint8_t type, uint16_t max_size, Usb::EpCallback cb);
    static void epReset();
    static void epSetStalled(uint8_t addr, bool stalled);
    static void epSetNak(uint8_t addr, bool nak);
    static bool epIsStalled(uint8_t addr);
    static uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len);
    static uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len);
    static void setDisconnected(bool disconnected);
};

#endif // USB_HARDWARE_H
