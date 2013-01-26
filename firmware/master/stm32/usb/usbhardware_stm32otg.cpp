#include "usb/usbhardware_stm32otg.h"
#include "usb/usbhardware.h"
#include "usb/usbcontrol.h"
#include "usb/usbcore.h"
#include "usb/usbdevice.h"

#include "hardware.h"
#include "macros.h"
#include "string.h"

using namespace Usb;
using namespace UsbHardwareStm32Otg;

/*
 * STM32 OTG implementation of UsbHardware.
 */
namespace UsbHardware {

void init(const UsbCore::Config & cfg)
{
    // turn on the peripheral clock
    RCC.AHBENR |= (1 << 12);

    // Enable VBUS sensing in device mode and power down the PHY
    OTG.global.GCCFG |= (1 << 19) |     // VBUSBSEN
                        (1 << 16);      // PWRDWN

    // Wait for AHB idle
    while (!(OTG.global.GRSTCTL & (1 << 31)))   // AHBIDL
        ;

    // Do core soft reset
    OTG.global.GRSTCTL |= (1 << 0); // CSRST
    while (OTG.global.GRSTCTL & (1 << 0))
        ;

    // Force peripheral only mode
    OTG.global.GUSBCFG |= (1 << 30) |   // FDMOD
                          (0xf << 10) | // TRDT max - TODO: optimize?
                          (1 << 7);     // PHYSEL

    // Full speed device
    OTG.device.DCFG |= 0x3; // DSPD

    // Restart the PHY clock
    OTG.PCGCCTL = 0;

    // Unmask interrupts for TX and RX
    OTG.device.DIEPMSK = 0;
    OTG.device.DOEPMSK = 0;
    OTG.device.DAINTMSK = 0;
    OTG.global.GINTMSK =
                        (1 << 31) |    // WUIM
                        (1 << 19) |    // OEPINT
                        (1 << 18) |    // IEPINT
                        (1 << 13) |    // ENUMDNEM
                        (1 << 12) |    // USBRST
                        (1 << 11) |    // USBSUSPM
                        (1 << 4)  |    // RXFLVLM
                        (cfg.enableSOF ? (1 << 3) : 0); // SOFM

    OTG.global.GINTSTS = 0xffffffff;    // clear any pending IRQs
    OTG.global.GAHBCFG |= 0x1;          // global interrupt enable
}

void deinit()
{
    // turn on the peripheral clock
    RCC.AHBENR &= ~(1 << 12);

    OTG.global.GCCFG = 0;
}

/*
 * Handle a USBRST signal from the usb hardware.
 * Flush TX/RX buffers, disable all OUT endpoints, and do preliminary setup on
 * EP0 - note the configuration of EP0's IN portion must happen after we
 * receive the ENUMDNE signal.
 */
void reset()
{
    // flush all TX FIFOs
    OTG.global.GRSTCTL = (1 << 10) | (1 << 5);
    while (OTG.global.GRSTCTL & (1 << 5))
        ;

    // flush RX FIFO
    OTG.global.GRSTCTL = (1 << 4);
    while (OTG.global.GRSTCTL & (1 << 4))
        ;

    // disable EPs, only if they're not already disabled.
    for (unsigned i = 0; i < 4; ++i) {

        // OUT EPs
        volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[i];
        if (ep.DOEPCTL & (1 << 31)) {
            ep.DOEPCTL = (1 << 30) | (1 << 27);
        }
        ep.DOEPTSIZ = 0;
        ep.DOEPINT = 0xff;  // clear any pending interrupts

        // IN EPs
        volatile USBOTG_IN_EP_t & inep = OTG.device.inEps[i];
        if (inep.DIEPCTL & (1 << 31)) {
            inep.DIEPCTL = (1 << 30) | (1 << 27);
        }
        inep.DIEPTSIZ = 0;
        inep.DIEPINT = 0xff;  // clear any pending interrupts
    }

    OTG.device.DAINTMSK = (1 << 16) | (1 << 0); // ep0 IN & OUT
    OTG.device.DIEPMSK = (1 << 0) | (1 << 4);   // xfer complete & ITTXFE
    OTG.device.DOEPMSK = (1 << 3) | (1 << 0);   // setup complete, and xfer complete

    OTG.global.GRXFSIZ = RX_FIFO_WORDS;
    fifoMemTop = RX_FIFO_WORDS;

    // Configure EP0 OUT
    doeptsiz[0] = (3 << 29) |   // STUPCNT_3
                  (1 << 19) |
                  (MAX_PACKET);

    OTG.device.outEps[0].DOEPTSIZ = doeptsiz[0];
    OTG.device.outEps[0].DOEPCTL |= ((1 << 31) |    // EPENA
                                    (1 << 27));    // SNAK

    // in the global FIFO map, after the global RX size is the
    // EP0 TX config
    uint16_t fifoDepthInWords = MAX_PACKET / 4;
    OTG.global.DIEPTXF0_HNPTXFSIZ = (fifoDepthInWords << 16) | fifoMemTop;
    fifoMemTop += fifoDepthInWords;
}

void setAddress(uint8_t addr)
{
    OTG.device.DCFG = (OTG.device.DCFG & ~0x07F0 /* DCFG_DAD */) | (addr << 4);
}

// Configure endpoint address and type & allocate FIFO memory for endpoint
void epINSetup(uint8_t addr, uint8_t type, uint16_t maxsize)
{
    addr &= 0x7f;

    uint16_t fifoDepthInWords = maxsize / 4;
    OTG.global.DIEPTXF[addr - 1] = (fifoDepthInWords << 16) | fifoMemTop;
    fifoMemTop += fifoDepthInWords;

    // NOTE: SNAK is avoided in order to allow ITTXFE ISRs to trigger
    // before we've written anything out
    volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];
    ep.DIEPCTL  = ((1 << 31) |     // EPENA
                   (1 << 28) |     // SD0PID
                   (addr << 22) |
                   (type << 18) |
                   (1 << 15) |     // USBEAP
                   maxsize);

    // clear pending interrupts & enable ISRs for this ep
    ep.DIEPINT = 0xff;
    OTG.device.DAINTMSK |= (1 << addr);
}

void epOUTSetup(uint8_t addr, uint8_t type, uint16_t maxsize)
{
    doeptsiz[addr] = (1 << 19) | (maxsize & 0x7f);

    volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[addr];
    ep.DOEPTSIZ = doeptsiz[addr];
    ep.DOEPCTL |= ((1 << 31) |     // EPENA
                   (1 << 28) |     // SD0PID
                   (1 << 26) |     // CNAK
                   (type << 18) |
                   (1 << 15) |     // USBEAP
                   maxsize);

    // clear pending interrupts & enable ISRs for this ep
    ep.DOEPINT = 0xff;
    OTG.device.DAINTMSK |= (1 << (addr + 16));
}

void epStall(uint8_t addr)
{
    const uint32_t stallbit = (1 << 21);
    if (isInEp(addr))
        OTG.device.inEps[addr & 0x7f].DIEPCTL |= stallbit;
    else
        OTG.device.outEps[addr].DOEPCTL |= stallbit;
}

void epClearStall(uint8_t addr)
{
    const uint32_t stallbit = (1 << 21);
    if (isInEp(addr)) {
        addr &= 0x7F;
        OTG.device.inEps[addr].DIEPCTL &= ~stallbit;
        OTG.device.inEps[addr].DIEPCTL |= (1 << 28);   // SD0PID
    } else {
        OTG.device.outEps[addr].DOEPCTL &= ~stallbit;
        OTG.device.outEps[addr].DOEPCTL |= (1 << 28);   // SD0PID
    }
}

bool epIsStalled(uint8_t addr)
{
    const uint32_t stallbit = (1 << 21);
    if (isInEp(addr))
        return (OTG.device.inEps[addr & 0x7f].DIEPCTL & stallbit) != 0;
    else
        return (OTG.device.outEps[addr].DOEPCTL & stallbit) != 0;
}

uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    addr &= 0x7F;
    volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];

    inEndpointStates[addr].buf = static_cast<const uint8_t*>(buf);
    inEndpointStates[addr].len = len;

    /*
     * As long as the data is in a contiguous buffer and there's room in the fifo,
     * multiple packet's worth of data can be sent automatically, arranged as
     * a series of max length packets, followed by the remainder in a short packet.
     *
     * For zero-length packets, size is left at 0 but packet count is set to 1.
     */
    if (len == 0) {
        ep.DIEPTSIZ = (1 << 19);
    }
    else {
        uint8_t pktcnt = (len + MAX_PACKET - 1) / MAX_PACKET;
        ep.DIEPTSIZ = (pktcnt << 19) | len;
    }
    ep.DIEPCTL |= (1 << 31) | (1 << 26);     // EPENA & CNAK
    OTG.device.DIEPEMPMSK |= (1 << addr);

    return len;
}

/*
 * Read directly from the fifo the RX data that most recently arrived.
 * Don't update counts or re-enable RXFLVL yet - this happens once the
 * packet is read by the actual client, at which point we're ready to
 * receive another packet.
 */
static void epReadFifo(uint16_t len)
{
    // there's only one shared rx fifo, so arbitrarily access via ep0
    uint32_t wordCount = (len + 3) / 4;
    volatile uint32_t *fifo = OTG.epFifos[0];
    uint32_t *buf32 = reinterpret_cast<uint32_t*>(&rxFifoBuf.bytes[0]);

    while (wordCount--) {
        *buf32++ = *fifo;
    }
    rxFifoBuf.len = len;
}

/*
 * Get the RX packet that most recently arrived.
 * If it's on EP0, we read directly from the fifo.
 * For any other endpoint, it should have already been buffered - read it from there.
 */
uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    len = MIN(len, rxFifoBuf.len);
    rxFifoBuf.len = 0;

    // copy the data out of our buffer
    // TODO: UsbDevice should provide its own endpoint buffers
    memcpy(buf, rxFifoBuf.bytes, len);

    // unmask RXFLVL & re-enable this endpoint for more RXing
    OTG.global.GINTMSK |= RXFLVL;

    volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[addr];
    ep.DOEPTSIZ = doeptsiz[addr];
    ep.DOEPCTL |= (1 << 31) | (1 << 26);    // EPENA & CNAK

    return len;
}

void rxflvlISR()
{
    /*
     * A new packet has arrived and is ready to start getting processed.
     *
     * In the case of both SETUP and OUT data, we need to read the data
     * out of the RX fifo immediately, and then once the core determines
     * the transaction is complete it will trigger OEPINT and we can
     * take action.
     */

//    Usb::SetupData *ctrlReq;
    uint32_t rxstsp = OTG.global.GRXSTSP;
    uint16_t pktsts = (rxstsp >> 17) & 0xf;
    uint16_t bcnt = (rxstsp >> 4) & 0x3ff;   // BCNT mask

    switch (pktsts) {

    case PktStsSetupData:
        UsbHardware::epReadFifo(bcnt);
#if 0
        // XXX: capture ep0 state for improved usbcontrol handling
        ctrlReq = (Usb::SetupData*)rxFifoBuf.bytes;

        // set the state of EP0 based on the direction of this setup request
        if (Usb::reqIsOUT(ctrlReq->bmRequestType) && ctrlReq->wLength > 0) {
            ep0Status = UsbControl::Ep0SetupOut;
        } else {
            ep0Status = UsbControl::Ep0SetupReady;
        }
#endif
        break;

    case PktStsOutData:
        if (bcnt > 0) {
            UsbHardware::epReadFifo(bcnt);
        }
        break;
    }
}

void inEpISR()
{
    /*
     * In ep activity - IEPINT indicates global out endpoint activity, must
     * read DAINT to see which endpoints activity actually occurred on.
     */

    uint16_t inEpInts = OTG.device.DAINT & OTG.device.DAINTMSK;
    for (unsigned i = 0; inEpInts != 0; ++i, inEpInts >>= 1) {

        volatile USBOTG_IN_EP_t & inep = OTG.device.inEps[i];
        uint32_t inEpInt = inep.DIEPINT & OTG.device.DIEPMSK;
        // only consider TXFE if DIEPEMPMSK is unmasked
        if (OTG.device.DIEPEMPMSK & (1 << i)) {
            inEpInt |= (1 << 7);
        }
        inep.DIEPINT = 0xff;

        /*
         * TXFE: transmit FIFO has room to write a packet.
         */
        if (inEpInt & (1 << 7)) {

            OTG.device.DIEPEMPMSK &= ~(1 << i);

            InEndpointState &eps = inEndpointStates[i];
            const uint32_t* buf32 = reinterpret_cast<const uint32_t*>(eps.buf);
            volatile uint32_t* fifo = OTG.epFifos[i];
            unsigned nwords = (eps.len + 3) / 4;
            while (nwords--) {
                *fifo = *buf32++;
            }
            eps.len = 0;
        }

        /*
         * XFRC: transmission complete
         */
        if (inEpInt & (1 << 0)) {
            if (i == 0) {
                UsbControl::controlRequest(0, TransactionIn);
            } else {
                UsbDevice::inEndpointCallback(i);
            }
        }

        /*
         * ITTXFE: IN token was received while the tx fifo was empty.
         * Allow the device to track when the host is "listening".
         */
        if (inEpInt & (1 << 4)) {
            if (i != 0) {
                UsbDevice::onINToken(i);
            }
        }
    }
}

void outEpISR()
{
    /*
     *  OUT ep activity - handles both SETUP and OUT events.
     */

    uint16_t outEpInts = (OTG.device.DAINT & OTG.device.DAINTMSK) >> 16;
    for (unsigned i = 0; outEpInts != 0; ++i, outEpInts >>= 1) {

        uint32_t outEpInt = OTG.device.outEps[i].DOEPINT & OTG.device.DOEPMSK;
        OTG.device.outEps[i].DOEPINT = 0xff;

        if (outEpInt & (1 << 3)) {  // setup complete
            UsbControl::controlRequest(0, TransactionSetup);
        }

        if (outEpInt & (1 << 0)) {  // OUT transfer complete
            if (i == 0) {
                UsbControl::controlRequest(0, TransactionIn);
            } else {
                // don't receive any more packets until UsbDevice reads this one
                OTG.global.GINTMSK &= ~RXFLVL;
                UsbDevice::outEndpointCallback(i);
            }
        }
    }
}

void disconnect()
{
    OTG.device.DCTL |= (1 << 1);    // SDIS
}

} // namespace UsbHardware

IRQ_HANDLER ISR_UsbOtg_FS()
{
    uint32_t status = OTG.global.GINTSTS & OTG.global.GINTMSK;

    if (status & USBRST) {
        UsbHardware::reset();
        UsbCore::reset();
        OTG.global.GINTSTS = USBRST;
    }

    if (status & ENUMDNE) {
        // must wait till enum is done to configure in ep0
        OTG.device.inEps[0].DIEPCTL = 0x0 | (1 << 27); // MPSIZ 64
        OTG.device.DCTL |= (1 << 8);    // CGINAK: Clear global IN NAK
        OTG.global.GINTSTS = ENUMDNE;
    }

    if (status & RXFLVL) {  // this bit is read-only
        UsbHardware::rxflvlISR();
    }

    if (status & IEPINT) {  // this bit is read-only
        UsbHardware::inEpISR();
    }

    if (status & OEPINT) {  // this bit is read-only
        UsbHardware::outEpISR();
    }

    if (status & USBSUSP) {
        UsbDevice::handleSuspend();
        OTG.global.GINTSTS = USBSUSP;
    }

    if (status & WKUPINT) {
        UsbDevice::handleResume();
        OTG.global.GINTSTS = WKUPINT;
    }

    if (status & SOF) {
        UsbDevice::handleStartOfFrame();
        OTG.global.GINTSTS = SOF;
    }
}
