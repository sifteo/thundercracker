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

    OTG.global.GRXFSIZ = RX_FIFO_WORDS;
    fifoMemTop = RX_FIFO_WORDS;

    for (unsigned i = 0; i < 4; ++i)
        OTG.device.outEps[i].DOEPCTL |= (1 << 27);  // set nak

    // Unmask interrupts for TX and RX
    OTG.global.GINTMSK =
                        (1 << 31) |    // WUIM
                        (1 << 18) |    // IEPINT
                        (1 << 13) |    // ENUMDNEM
                        (1 << 11) |    // USBSUSPM
                        (1 << 4)  |    // RXFLVLM
                        (1 << 3);      // SOFM
    OTG.device.DAINTMSK = 0xf;
    OTG.device.DIEPMSK = 0x1;

    OTG.global.GINTSTS = 0xffffffff;    // clear any pending IRQs
    OTG.global.GAHBCFG |= 0x1;          // global interrupt enable
    // don't bother with DOEPMSK - we handle all OUT endpoint activity via the
    // RXFLVL interrupt
}

void setAddress(uint8_t addr)
{
    OTG.device.DCFG = (OTG.device.DCFG & ~0x07F0 /* DCFG_DAD */) | (addr << 4);
}

// Configure endpoint address and type & allocate FIFO memory for endpoint
void epSetup(uint8_t addr, uint8_t type, uint16_t maxsize)
{
    // handle control endpoint specially
    if ((addr & 0x7f) == 0) {

        // Configure IN
        if (maxsize >= 64)
            OTG.device.inEps[0].DIEPCTL = 0x0; // MPSIZ 64
        else if (maxsize >= 32)
            OTG.device.inEps[0].DIEPCTL = 0x1; // MPSIZ 32
        else if (maxsize >= 16)
            OTG.device.inEps[0].DIEPCTL = 0x2; // MPSIZ 16
        else
            OTG.device.inEps[0].DIEPCTL = 0x3; // MPSIZ 8

        OTG.device.inEps[0].DIEPTSIZ = maxsize & 0x7f;
        OTG.device.inEps[0].DIEPCTL |= ((1 << 31) |    // EPENA
                                        (1 << 27));    // SNAK

        // Configure OUT
        doeptsiz[0] = (3 << 29) |   // STUPCNT_3
                      (1 << 19) |
                      (maxsize & 0x7f);

        OTG.device.outEps[0].DOEPTSIZ = doeptsiz[0];
        OTG.device.outEps[0].DOEPCTL |= ((1 << 31) |    // EPENA
                                        (1 << 27));    // SNAK

        // in the global FIFO map, after the global RX size is the
        // EP0 TX config
        uint16_t fifoDepthInWords = maxsize / 4;
        OTG.global.DIEPTXF0_HNPTXFSIZ = (fifoDepthInWords << 16) | fifoMemTop;
        fifoMemTop += fifoDepthInWords;

        return;
    }

    if (isInEp(addr)) {

        addr &= 0x7f;

        uint16_t fifoDepthInWords = maxsize / 4;
        OTG.global.DIEPTXF[addr] = (fifoDepthInWords << 16) | fifoMemTop;
        fifoMemTop += fifoDepthInWords;

        volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];
        ep.DIEPTSIZ = maxsize & 0x7f;
        ep.DIEPCTL  = ((1 << 31) |     // EPENA
                       (1 << 28) |     // SD0PID
                       (1 << 27) |     // SNAK
                       (addr << 22) |
                       (type << 18) |
                       (1 << 15) |     // USBEAP
                       maxsize);
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
    }
}

void epReset()
{
    // The core resets the endpoints automatically on reset.
    // TODO - anything else we need to do here?
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
static void epReadFifo(uint8_t addr, void *buf, uint16_t len)
{
    uint32_t *buf32 = static_cast<uint32_t*>(buf);

    volatile uint32_t *fifo = OTG.epFifos[addr];
    int i;
    for (i = len; i >= 4; i -= 4)
        *buf32++ = *fifo;

    if (i) {
        uint32_t extra = *fifo;
        memcpy(buf32, &extra, i);
    }
}

/*
 * Get the RX packet that most recently arrived.
 * If it's on EP0, we read directly from the fifo.
 * For any other endpoint, it should have already been buffered - read it from there.
 */
uint16_t epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    len = MIN(len, rxbcnt);
    rxbcnt -= len;

    if (addr == 0)
        epReadFifo(addr, buf, len);
    else
        memcpy(buf, packetBuf, len);

    // restore state
    volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[addr];
    ep.DOEPTSIZ = doeptsiz[addr];
    ep.DOEPCTL |= (1 << 31) | (1 << 26);  // EPENA & CNAK
    OTG.global.GINTMSK |= (1 << 4); // unmask RXFLVL
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

    const uint32_t ENUMDNE = 1 << 13;
    if (status & ENUMDNE) {
        OTG.global.GINTSTS = ENUMDNE;
        fifoMemTop = RX_FIFO_WORDS;
        UsbCore::reset();
    }

    const uint32_t RXFLVL = 1 << 4; // Receive FIFO non-empty
    if (status & RXFLVL) {
        // RXFLVL is read-only in GINTSTS
        uint32_t rxstsp = OTG.global.GRXSTSP;

        Transaction txn;
        switch (rxstsp & (0xf << 17)) {  // PKTSTS mask
        case 0x6 << 17:
            txn = TransactionSetup;
            break;
        case 0x2 << 17:
            txn = TransactionOut;
            break;
        default:
            return;
        }

        // Save packet size for epReadPacket()
        // don't assign to our persistent member until we know it's good
        uint16_t tmpcnt = (rxstsp >> 4) & 0x3ff;  // BCNT mask

        if (tmpcnt > 0) {

            uint8_t ep = rxstsp & 0xf; // EPNUM mask

            // mask RXFLVL until we've read the data from the fifo
            OTG.global.GINTMSK &= ~RXFLVL;
            rxbcnt = tmpcnt;

            /*
             * Control requests get handled directly within this ISR, so we don't
             * need to buffer data.
             *
             * Data endpoints may be read at some later date, so we get the data
             * out of the fifo and buffer it for later.
             */
            if (ep == 0) {
                UsbControl::controlRequest(ep, txn);
            }
            else {
                UsbHardware::epReadFifo(ep, UsbHardwareStm32Otg::packetBuf, sizeof UsbHardwareStm32Otg::packetBuf);
                UsbDevice::outEndpointCallback(ep);
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
