#ifndef USB_HARDWARE_H
#define USB_HARDWARE_H

#include <stdint.h>

/*
 * Platform specific interface to perform hardware related USB tasks.
 */
namespace UsbHardware
{
    void init();
    void setAddress(uint8_t addr);
    void epSetup(uint8_t addr, uint8_t type, uint16_t max_size);
    void epReset();
    void epSetStalled(uint8_t addr, bool stalled);
    void epSetNak(uint8_t addr, bool nak);
    bool epIsStalled(uint8_t addr);
    bool epTxInProgress(uint8_t addr);
    uint16_t epTxWordsAvailable(uint8_t addr);
    uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len);
    uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len);
    void setDisconnected(bool disconnected);
}

#endif // USB_HARDWARE_H
