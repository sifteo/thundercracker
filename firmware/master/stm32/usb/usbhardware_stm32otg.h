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

    // The number of received bytes for the most recently received packet
    uint16_t numBufferedBytes;

    /*
     * In order to tick along the hardware's state machine, we must read packets
     * out of usb ram as soon as they arrive. Stash them here until the application
     * reads them out.
     *
     * TODO: the application should provide this buffer, and we should provide a
     * mechanism for it to tell us once it has consumed it such that we can
     * start receiving the next one.
     */
    uint8_t packetBuf[MAX_PACKET];

    // keep track of our usb ram allocation
    uint16_t fifoMemTop;

    // We keep a backup copy of the out endpoint size registers to restore them
    // after a transaction.
    uint32_t doeptsiz[4];
}

#endif // STM32F10XOTG_H
