#ifndef STM32F10XOTG_H
#define STM32F10XOTG_H

#include "usb/usbdefs.h"

class Stm32f10xOtg
{
public:
    Stm32f10xOtg();

    void init();
    void setAddress(uint8_t addr);
    void epSetup(uint8_t addr, uint8_t type, uint16_t maxsize);
    void epReset();
    void epSetStall(uint8_t addr, bool stall);
    void epSetNak(uint8_t addr, bool nak);
    bool epIsStalled(uint8_t addr);
    bool epTxInProgress(uint8_t addr);
    uint16_t epTxWordsAvailable(uint8_t addr);
    uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len);
    uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len);
    void isr();
    void setDisconnected(bool disconnected);

private:
    static const unsigned RX_FIFO_WORDS = 128;
    static const unsigned MAX_PACKET = 64;

    // Received packet size for each endpoint.
    // Gets assigned in the ISR from GRXSTSP and used in epReadPacket()
    uint16_t rxbcnt;

    uint16_t fifoMemTop;
    uint16_t fifoMemTopEp0;
    uint8_t forceNak[4];

    // We keep a backup copy of the out endpoint size registers to restore them
    // after a transaction.
    uint32_t doeptsiz[4];
};

#endif // STM32F10XOTG_H
