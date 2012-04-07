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

void init()
{
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
                        (1 << 3);      // SOFM

    OTG.global.GINTSTS = 0xffffffff;    // clear any pending IRQs
    OTG.global.GAHBCFG |= 0x1;          // global interrupt enable
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
    OTG.global.GRSTCTL = (1 << 11) | (1 << 5);
    while (OTG.global.GRSTCTL & (1 << 5))
        ;

    // flush RX FIFO
    OTG.global.GRSTCTL = (1 << 4);
    while (OTG.global.GRSTCTL & (1 << 4))
        ;

    // disable OUT EPs, only if they're not already disabled.
    for (unsigned i = 0; i < 4; ++i) {
        volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[i];
        if (ep.DOEPCTL & (1 << 31))
            ep.DOEPCTL = (1 << 30) | (1 << 27);
        else
            ep.DOEPCTL = 0;
        ep.DOEPTSIZ = 0;
        ep.DOEPINT = 0xff;  // clear any pending interrupts
    }

    OTG.device.DAINTMSK = (1 << 16) | (1 << 0); // ep0 IN & OUT
    OTG.device.DIEPMSK = 0x1;                   // xfer complete
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
void epSetup(uint8_t addr, uint8_t type, uint16_t maxsize)
{
    // EP0 gets handled during reset
    if ((addr & 0x7f) == 0)
        return;

    if (isInEp(addr)) {

        addr &= 0x7f;

        uint16_t fifoDepthInWords = maxsize / 4;
        OTG.global.DIEPTXF[addr - 1] = (fifoDepthInWords << 16) | fifoMemTop;
        fifoMemTop += fifoDepthInWords;

        volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];
        ep.DIEPTSIZ = maxsize;
        ep.DIEPCTL  = ((1 << 31) |     // EPENA
                       (1 << 28) |     // SD0PID
                       (1 << 27) |     // SNAK
                       (addr << 22) |
                       (type << 18) |
                       (1 << 15) |     // USBEAP
                       maxsize);

        // clear pending interrupts & enable ISRs for this ep
        ep.DIEPINT = 0xff;
        OTG.device.DAINTMSK |= (1 << addr);
    }
    else {

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
}

void epSetStalled(uint8_t addr, bool stall)
{
    const uint32_t stallbit = (1 << 21);

    if (addr == 0) {
        if (stall)
            OTG.device.inEps[addr].DIEPCTL |= stallbit;
        else
            OTG.device.inEps[addr].DIEPCTL &= ~stallbit;
    }

    if (isInEp(addr)) {

        addr &= 0x7F;

        if (stall) {
            OTG.device.inEps[addr].DIEPCTL |= stallbit;
        }
        else {
            OTG.device.inEps[addr].DIEPCTL &= ~stallbit;
            OTG.device.inEps[addr].DIEPCTL |= (1 << 28);   // SD0PID
        }
    }
    else {
        if (stall) {
            OTG.device.outEps[addr].DOEPCTL |= stallbit;
        }
        else {
            OTG.device.outEps[addr].DOEPCTL &= ~stallbit;
            OTG.device.outEps[addr].DOEPCTL |= (1 << 28);   // SD0PID
        }
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

void epSetNak(uint8_t addr, bool nak)
{
    // n/a for IN endpoints
    if (isInEp(addr))
        return;

    // set or clear nak accordingly
    OTG.device.outEps[addr].DOEPCTL |= (nak ? (1 << 27) : (1 << 26));
}

uint16_t epTxWordsAvailable(uint8_t addr)
{
    // n/a for OUT endpoints
    if (!isInEp(addr))
        return 0;

    return OTG.device.inEps[addr & 0x7f].DTXFSTS & 0xffff;
}

bool epTxInProgress(uint8_t addr)
{
    // check the packet count in the IN endpoint
    uint16_t pktcnt = (OTG.device.inEps[addr & 0x7f].DIEPTSIZ >> 19) & 0x3ff;
    return pktcnt > 0;
}

uint16_t epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    addr &= 0x7F;
    volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];

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
        uint8_t pktcnt = (len - 1 + MAX_PACKET) / MAX_PACKET;
        ep.DIEPTSIZ = (pktcnt << 19) | len;
    }
    ep.DIEPCTL |= (1 << 31) | (1 << 26);     // EPENA & CNAK

    // Copy buffer to endpoint FIFO
    const uint32_t* buf32 = static_cast<const uint32_t*>(buf);
    volatile uint32_t* fifo = OTG.epFifos[addr];
    for (int i = len; i > 0; i -= 4)
        *fifo = *buf32++;
    return len;
}

/*
 * Read directly from the fifo the RX data that most recently arrived.
 * Don't update counts or re-enable RXFLVL yet - this happens once the
 * packet is read by the actual client, at which point we're ready to
 * receive another packet.
 */
static void epReadFifo(void *buf, uint16_t len)
{
    uint32_t *buf32 = static_cast<uint32_t*>(buf);
    len = MIN(len, numBufferedBytes);

    volatile uint32_t *fifo = OTG.epFifos[0];
    uint32_t wordCount = (len + 3) / 4;
    while (wordCount--)
        *buf32++ = *fifo;
}

/*
 * Get the RX packet that most recently arrived.
 * If it's on EP0, we read directly from the fifo.
 * For any other endpoint, it should have already been buffered - read it from there.
 */
uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    len = MIN(len, numBufferedBytes);
    numBufferedBytes -= len;

    // copy the data out of our buffer
    // TODO: UsbDevice should provide its own endpoint buffers
    memcpy(buf, packetBuf, len);

    // unmask RXFLVL & re-enable this endpoint for more RXing
    OTG.global.GINTMSK |= (1 << 4);

    volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[addr];
    ep.DOEPTSIZ = doeptsiz[addr];
    ep.DOEPCTL |= (1 << 31) | (1 << 26);    // EPENA & CNAK

    return len;
}

void setDisconnected(bool disconnected)
{
    const uint32_t sdis = (1 << 1);

    if (disconnected)
        OTG.device.DCTL |= sdis;
    else
        OTG.device.DCTL &= ~sdis;
}

} // namespace UsbHardware

IRQ_HANDLER ISR_UsbOtg_FS()
{
    uint32_t status = OTG.global.GINTSTS;

    const uint32_t USBRST = 1 << 12;
    if (status & USBRST) {
        UsbHardware::reset();
        UsbCore::reset();
        OTG.global.GINTSTS = USBRST;
    }

    const uint32_t ENUMDNE = 1 << 13;
    if (status & ENUMDNE) {

        // must wait till enum is done to configure in ep0
        OTG.device.inEps[0].DIEPCTL = 0x0 | (1 << 27); // MPSIZ 64
        OTG.global.GINTSTS = ENUMDNE;
    }

    const uint32_t RXFLVL = 1 << 4; // Receive FIFO non-empty
    if (status & RXFLVL) {

        // RXFLVL is read-only in GINTSTS
        uint32_t rxstsp = OTG.global.GRXSTSP;
        uint16_t pktsts = (rxstsp >> 17) & 0xf;

        /*
         * For any packet status configurations that indicate that data has arrived,
         * read the data out of the fifo. Once this happens, we'll either get a
         * SETUP complete or an OUT complete event, both of which get handled
         * via OEPINT.
         */
        if (pktsts == PktStsSetupData || pktsts == PktStsOutData) {
            uint16_t bcnt = (rxstsp >> 4) & 0x3ff;  // BCNT mask
            if (bcnt > 0) {
                /*
                 * mask RXFLVL until this packet gets consumed by the application.
                 * this allows the hw to fill the remaining usb ram with OUT packets,
                 * but we won't process them until the application is ready.
                 * it will NAK in the case the usb ram fills up.
                 */
                OTG.global.GINTMSK &= ~RXFLVL;
                numBufferedBytes = bcnt;
                UsbHardware::epReadFifo(packetBuf, sizeof packetBuf);
            }
        }
    }

    /*
     * In ep activity - IEPINT indicates global out endpoint activity, must
     * read DAINT to see which endpoints activity actually occurred on.
     */
    const uint32_t IEPINT = 1 << 18;    // this bit is read-only
    if (status & IEPINT) {
        uint16_t inEpInts = OTG.device.DAINT & 0xffff;
        // TODO: could micro optimize here with CLZ and friends
        for (unsigned i = 0; inEpInts != 0; ++i, inEpInts >>= 1) {
            volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[i];
            // only really interested in XFRC to indicate TX complete
            if (ep.DIEPINT & 0x1) {
                if (i != 0)
                    UsbDevice::inEndpointCallback(i);
                ep.DIEPINT = 0x1;
            }
        }
    }

    /*
     *  OUT ep activity - handles both SETUP and OUT events.
     */
    const uint32_t OEPINT = 1 << 19;    // this bit is read-only
    if (status & OEPINT) {
        uint16_t outEpInts = (OTG.device.DAINT >> 16) & 0xffff;
        for (unsigned i = 0; outEpInts != 0; ++i, outEpInts >>= 1) {

            volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[i];

            if (ep.DOEPINT & (1 << 3)) {    // setup complete
                UsbControl::controlRequest(0, TransactionSetup);
                ep.DOEPINT = (1 << 3);
            }

            if (ep.DOEPINT & 0x1) {         // OUT transfer complete
                // TODO: update UsbControl handler to determine in/out stage
                // based on previous state
                if (i == 0)
                    UsbControl::controlRequest(0, TransactionIn);
                else
                    UsbDevice::outEndpointCallback(i);
                ep.DOEPINT = 0x1;
            }
        }
    }

    const uint32_t usbsusp = 1 << 11;
    if (status & usbsusp) {
        UsbDevice::handleSuspend();
        OTG.global.GINTSTS = usbsusp;
    }

    const uint32_t wkupint = 1 << 31;
    if (status & wkupint) {
        UsbDevice::handleResume();
        OTG.global.GINTSTS = wkupint;
    }

    const uint32_t sof = 1 << 3;
    if (status & sof) {
        UsbDevice::handleStartOfFrame();
        OTG.global.GINTSTS = sof;
    }
}
