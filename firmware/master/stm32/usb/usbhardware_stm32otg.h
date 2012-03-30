#ifndef STM32F10XOTG_H
#define STM32F10XOTG_H

#include <stdint.h>

/*
 * Members specific to the STM32 OTG implementation of UsbHardware.
 */
namespace UsbHardwareStm32Otg
{
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
}

#endif // STM32F10XOTG_H
