#include "stm32f10xotg.h"
#include "usb/usbd.h"
#include "usb/usbdriver.h"
#include "usb/usbcontrol.h"

#include "hardware.h"
#include "macros.h"
#include "string.h"

using namespace Usb;

Stm32f10xOtg::Stm32f10xOtg() :
    rxbcnt(0),
    fifoMemTop(0),
    fifoMemTopEp0(0)
{}

void Stm32f10xOtg::init()
{
    OTG.global.GINTSTS = (1 << 1); // MMIS - mode mismatch
    OTG.global.GUSBCFG |= (1 << 7); // PHYSEL

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
                          (0xf << 10);  // TRDT max - TODO: optimize?

    // Full speed device
    OTG.device.DCFG |= 0x3; // DSPD

    // Restart the PHY clock
    OTG.PCGCCTL = 0;

    OTG.global.GRXFSIZ = RX_FIFO_SIZE;
    fifoMemTop = RX_FIFO_SIZE;

    for (unsigned i = 0; i < 4; ++i)
        OTG.device.outEps[i].DOEPCTL |= (1 << 27);  // set nak

    // Unmask interrupts for TX and RX
    OTG.global.GAHBCFG |= 0x1;  // GINT
    OTG.global.GINTMSK =
                        (1 << 31) |    // WUIM
                        (1 << 18) |    // IEPINT
                        (1 << 13) |    // ENUMDNEM
                        (1 << 11) |    // USBSUSPM
                        (1 << 4)  |    // RXFLVLM
                        (1 << 3);      // SOFM
    OTG.device.DAINTMSK = 0xf;
    OTG.device.DIEPMSK = 0x1;
    // don't bother with DOEPMSK - we handle all OUT endpoint activity via the
    // RXFLVL interrupt
}

void Stm32f10xOtg::setAddress(uint8_t addr)
{
    OTG.device.DCFG = (OTG.device.DCFG & ~0x07F0 /* DCFG_DAD */) | (addr << 4);
}

void Stm32f10xOtg::epSetup(uint8_t addr, uint8_t type, uint16_t maxsize)
{
    // Configure endpoint address and type & allocate FIFO memory for endpoint
    addr &= 0x7f;

    // handle control endpoint specially
    if (addr == 0) {

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

        OTG.global.DIEPTXF0_HNPTXFSIZ = ((maxsize / 4) << 16) | RX_FIFO_SIZE;
        fifoMemTop += maxsize / 4;
        fifoMemTopEp0 = fifoMemTop;

        return;
    }


    if (isInEp(addr)) {

        OTG.global.DIEPTXF[addr] = ((maxsize / 4) << 16) | fifoMemTop;
        fifoMemTop += maxsize / 4;

        volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];
        ep.DIEPTSIZ = maxsize & 0x7f;
        ep.DIEPCTL |=  ((1 << 31) |     // EPENA
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

void Stm32f10xOtg::epReset()
{
    // The core resets the endpoints automatically on reset.
    fifoMemTop = fifoMemTopEp0;
}

void Stm32f10xOtg::epSetStall(uint8_t addr, bool stall)
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

bool Stm32f10xOtg::epIsStalled(uint8_t addr)
{
    const uint32_t stallbit = (1 << 21);

    if (isInEp(addr))
        return (OTG.device.inEps[addr & 0x7f].DIEPCTL & stallbit) != 0;
    else
        return (OTG.device.outEps[addr].DOEPCTL & stallbit) != 0;
}

void Stm32f10xOtg::epSetNak(uint8_t addr, bool nak)
{
    // n/a for IN endpoints
    if (isInEp(addr))
        return;

    forceNak[addr] = nak;
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

uint16_t Stm32f10xOtg::epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    addr &= 0x7F;
    volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[addr];

    // endpoint already enabled? no no
    if (ep.DIEPTSIZ & (1 << 19))
        return 0;

    // Enable endpoint for transmission
    ep.DIEPTSIZ = (1 << 19) | len;
    ep.DIEPCTL = (1 << 31) |    // EPENA
                 (1 << 26);     // CNAK

    // Copy buffer to endpoint FIFO
    const uint32_t* buf32 = static_cast<const uint32_t*>(buf);
    volatile uint32_t* fifo = OTG.epFifos[addr];
    for (int i = len; i > 0; i -= 4)
        *fifo++ = *buf32++;

    return len;
}

uint16_t Stm32f10xOtg::epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    uint32_t *buf32 = static_cast<uint32_t*>(buf);

    len = MIN(len, rxbcnt);
    rxbcnt -= len;

    volatile uint32_t *fifo = OTG.epFifos[addr];
    int i;
    for (i = len; i >= 4; i -= 4)
        *buf32++ = *fifo++;

    if (i) {
        uint32_t extra = *fifo;
        memcpy(buf32, &extra, i);
    }

    // restore state
    volatile USBOTG_OUT_EP_t & ep = OTG.device.outEps[addr];
    ep.DOEPTSIZ = doeptsiz[addr];
    ep.DOEPCTL |= (1 << 31) |   // EPENA
                  (forceNak[addr] ? (1 << 27) : (1 << 26));
    OTG.global.GINTMSK &= ~(1 << 4); // re-enable Receive FIFO non-empty
    return len;
}

void Stm32f10xOtg::isr()
{
    uint32_t status = OTG.global.GINTSTS;

    const uint32_t enumdne = 1 << 13;
    if (status & enumdne) {
        OTG.global.GINTSTS = enumdne;
        fifoMemTop = RX_FIFO_SIZE;
#if 0
        uint8_t enumeratedSpeed = (OTG.device.DSTS >> 1) & 0x3;
        // ensure we enumerated at full speed
        if (enumeratedSpeed != 0x3) {
            for (;;)
                ;
        }
#endif
        Usbd::reset();
    }

    const uint32_t RXFLVL = 1 << 4; // Receive FIFO non-empty
    if (status & RXFLVL) {
        // RXFLVL is read-only in GINTSTS
        uint32_t rxstsp = OTG.global.GRXSTSP;
        uint32_t pktsts = rxstsp & (0xf << 17);  // PKTSTS mask

        Transaction txn;
        switch (pktsts) {
        case 0x6 << 17:
            txn = TransactionSetup;
            break;
        case 0x2 << 17:
            txn = TransactionOut;
            break;
        default:
            return;
        }

        uint8_t ep = rxstsp & 0xf; // EPNUM mask

        // Save packet size for epReadPacket()
        rxbcnt = (rxstsp & (0x7ff << 4)) >> 4;  // BCNT mask

        if (rxbcnt == 0) {

        }
        else {
            // mask RXFLVL until we've read the data from the fifo
            OTG.global.GINTMSK |= RXFLVL;

            // call back user handler to read the data via epReadPacket()
            if (ep == 0) {
                UsbControl::controlRequest(ep, txn);
            }
            else {
                UsbDriver::outEndpointCallback(ep);
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
            if (inEpInts & 1) {
                volatile USBOTG_IN_EP_t & ep = OTG.device.inEps[i];
                // only really interested in XFRC to indicate TX complete
                if (ep.DIEPINT & 0x1) {
                    UsbDriver::inEndpointCallback(i);
                    ep.DIEPINT = 0x1;
                }
            }
        }
    }

    const uint32_t usbsusp = 1 << 11;
    if (status & usbsusp) {
        UsbDriver::handleSuspend();
        OTG.global.GINTSTS = usbsusp;
    }

    const uint32_t wkupint = 1 << 31;
    if (status & wkupint) {
        UsbDriver::handleResume();
        OTG.global.GINTSTS = wkupint;
    }

    const uint32_t sof = 1 << 3;
    if (status & sof) {
        UsbDriver::handleStartOfFrame();
        OTG.global.GINTSTS = sof;
    }
}

void Stm32f10xOtg::setDisconnected(bool disconnected)
{
    const uint32_t sdis = (1 << 1);

    if (disconnected)
        OTG.device.DCTL |= sdis;
    else
        OTG.device.DCTL &= ~sdis;
}
