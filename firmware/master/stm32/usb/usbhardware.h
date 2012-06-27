#ifndef USB_HARDWARE_H
#define USB_HARDWARE_H

#include <stdint.h>

/*
 * Platform specific interface to perform hardware related USB tasks.
 */
namespace UsbHardware
{
    static const unsigned MAX_PACKET = 64;

    void init();
    void reset();

    void setAddress(uint8_t addr);
    void epINSetup(uint8_t addr, uint8_t type, uint16_t max_size);
    void epOUTSetup(uint8_t addr, uint8_t type, uint16_t max_size);

    void epStall(uint8_t addr);
    void epClearStall(uint8_t addr);
    bool epIsStalled(uint8_t addr);

    uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len);
    uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len);

    void setDisconnected(bool disconnected);
}

#endif // USB_HARDWARE_H
