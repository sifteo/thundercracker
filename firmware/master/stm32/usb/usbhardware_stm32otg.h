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

    enum PacketStatus {
        PktStsGlobalNak     = 1,
        PktStsOutData       = 2,
        PktStsOutComplete   = 3,
        PktStsSetupComplete = 4,
        PktStsSetupData     = 6
    };

    // Received packet size for each endpoint.
    // Gets assigned in the ISR from GRXSTSP and used in epReadPacket()
    uint16_t rxbcnt;

    /*
     * this is a bit of a hack for now - if we don't copy packets from the
     * rx fifo immediately in the ISR, I've observed strange things happening
     * when there's some delay between the time we receive the packet & the time
     * we read it out.
     *
     * revisit this and kill it.
     */
    uint8_t packetBuf[MAX_PACKET];

    // keep track of our usb ram allocation
    uint16_t fifoMemTop;

    // We keep a backup copy of the out endpoint size registers to restore them
    // after a transaction.
    uint32_t doeptsiz[4];
}

#endif // STM32F10XOTG_H
