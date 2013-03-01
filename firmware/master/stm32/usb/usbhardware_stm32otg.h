#ifndef STM32F10XOTG_H
#define STM32F10XOTG_H

#include "usbhardware.h"

#include <stdint.h>

/*
 * Members specific to the STM32 OTG implementation of UsbHardware.
 */
namespace UsbHardwareStm32Otg
{
    static const unsigned RX_FIFO_WORDS = 128;

    enum PacketStatus {
        PktStsGlobalNak     = 1,
        PktStsOutData       = 2,
        PktStsOutComplete   = 3,
        PktStsSetupComplete = 4,
        PktStsSetupData     = 6
    };

    enum Status {
        CMOD            = (1 << 0),     // Current mode of operation
        MMIS            = (1 << 1),     // Mode mismatch interrupt
        OTGINT          = (1 << 2),     // OTG interrupt
        SOF             = (1 << 3),     // Start of frame
        RXFLVL          = (1 << 4),     // RxFIFO non-empty
        NPTXFE          = (1 << 5),     // Non-periodic TxFIFO empty
        GINAKEFF        = (1 << 6),     // Global IN non-periodic NAK effective
        GONAKEFF        = (1 << 7),     // Global OUT NAK effective
        ESUSP           = (1 << 10),    // Early suspend
        USBSUSP         = (1 << 11),    // USB suspend
        USBRST          = (1 << 12),    // USB reset
        ENUMDNE         = (1 << 13),    // Enumeration done
        ISOODRP         = (1 << 14),    // Isochronous OUT packet dropped interrupt
        EOPF            = (1 << 15),    // End of periodic frame interrupt
        IEPINT          = (1 << 18),    // IN endpoint interrupt
        OEPINT          = (1 << 19),    // OUT endpoint interrupt
        IISOIXFR        = (1 << 20),    // Incomplete isochronous IN transfer
        INCOMPISOOUT    = (1 << 21),    // Incomplete isochronous OUT transfer
        HPRTINT         = (1 << 24),    // Host port interrupt
        HCINT           = (1 << 25),    // Host channels interrupt
        PTXFE           = (1 << 26),    // Periodic TxFIFO empty
        CIDSCHG         = (1 << 28),    // Connector ID status change
        DISCINT         = (1 << 29),    // Disconnect detected interrupt
        SRQINT          = (1 << 30),    // Session request/new session detected interrupt
        WKUPINT         = (1 << 31)     // Resume/remote wakeup detected interrupt
    };

    /*
     * In order to tick along the hardware's state machine, we must read packets
     * out of usb ram as soon as they arrive. Stash them here until the application
     * reads them out.
     *
     * TODO: the application should provide this buffer, and we should provide a
     * mechanism for it to tell us once it has consumed it such that we can
     * start receiving the next one.
     */
    struct PacketBuf {
        uint8_t bytes[UsbHardware::MAX_PACKET];
        uint16_t len;
    };
    PacketBuf rxFifoBuf;


    struct InEndpointState {
        const uint8_t *buf;
        uint16_t len;
    };

    // keeping only 2 of these is cheating a little, but we know we're only
    // using EP0 and EP1 so we can save a tiny bit of RAM
    InEndpointState inEndpointStates[2];

    // keep track of our usb ram allocation
    uint16_t fifoMemTop;

    // We keep a backup copy of the out endpoint size registers to restore them
    // after a transaction.
    uint32_t doeptsiz[4];
}

#endif // STM32F10XOTG_H
