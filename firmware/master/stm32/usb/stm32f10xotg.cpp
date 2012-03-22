#include "stm32f10xotg.h"
#include "usb/usbd.h"
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
    while (!(OTG.global.GRSTCTL & (1 << 13)))   // AHBIDL
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
}

void Stm32f10xOtg::setAddress(uint8_t addr)
{
    OTG.device.DCFG = (OTG.device.DCFG & ~0x07F0 /* DCFG_DAD */) | (addr << 4);
}

void Stm32f10xOtg::epSetup(uint8_t addr, uint8_t type, uint16_t maxsize, epCallback cb)
{
    // Configure endpoint address and type. Allocate FIFO memory for
    // endpoint. Install callback funciton.
    addr &= 0x7f;

    // handle control endpoint specially
    if (addr == 0) {

        // Configure IN
        if (maxsize >= 64)
            OTG.inEps[0].DIEPCTL = 0x0; // MPSIZ_64
        else if (maxsize >= 32)
            OTG.inEps[0].DIEPCTL = 0x1; // MPSIZ_32
        else if (maxsize >= 16)
            OTG.inEps[0].DIEPCTL = 0x2; // MPSIZ_16
        else
            OTG.inEps[0].DIEPCTL = 0x3; // MPSIZ_8

        OTG.inEps[0].DIEPTSIZ = maxsize & 0x7f;
        OTG.inEps[0].DIEPCTL |= ((1 << 31) |    // EPENA
                                 (1 << 27));    // SNAK

        // Configure OUT
        doeptsiz[0] = (1 << 29) |   // STUPCNT_1
                      (1 << 19) |
                      (maxsize & 0x7f);

        OTG.outEps[0].DOEPTSIZ = doeptsiz[0];
        OTG.outEps[0].DOEPCTL |= ((1 << 31) |    // EPENA
                                  (1 << 27));    // SNAK

        OTG.global.DIEPTXF0_HNPTXFSIZ = ((maxsize / 4) << 16) | RX_FIFO_SIZE;
        fifoMemTop += maxsize / 4;
        fifoMemTopEp0 = fifoMemTop;

        return;
    }


    if (isInEp(addr)) {

        OTG.global.DIEPTXF[addr] = ((maxsize / 4) << 16) | fifoMemTop;
        fifoMemTop += maxsize / 4;

        volatile USBOTG_IN_EP_t & ep = OTG.inEps[addr];
        ep.DIEPTSIZ = maxsize & 0x7f;
        ep.DIEPCTL |=  ((1 << 31) |     // EPENA
                        (1 << 28) |     // SD0PID
                        (1 << 27) |     // SNAK
                        (addr << 22) |
                        (type << 18) |
                        (1 << 15) |     // USBEAP
                        maxsize);

        if (cb) {
//            _usbd_device.
//                    user_callback_ctr[addr][USB_TRANSACTION_IN] =
//                    (void *)callback;
        }
    }
    else {

        doeptsiz[addr] = (1 << 19) | (maxsize & 0x7f);

        volatile USBOTG_OUT_EP_t & ep = OTG.outEps[addr];
        ep.DOEPTSIZ = doeptsiz[addr];
        ep.DOEPCTL |= ((1 << 31) |     // EPENA
                       (1 << 28) |     // SD0PID
                       (1 << 26) |     // CNAK
                       (type << 18) |
                       (1 << 15) |     // USBEAP
                       maxsize);

        if (cb) {
//            _usbd_device.
//                    user_callback_ctr[addr][USB_TRANSACTION_OUT] =
//                    (void *)callback;
        }
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
            OTG.inEps[addr].DIEPCTL |= stallbit;
        else
            OTG.inEps[addr].DIEPCTL &= ~stallbit;
    }

    if (isInEp(addr)) {

        addr &= 0x7F;

        if (stall) {
            OTG.inEps[addr].DIEPCTL |= stallbit;
        }
        else {
            OTG.inEps[addr].DIEPCTL &= ~stallbit;
            OTG.inEps[addr].DIEPCTL |= (1 << 28);   // SD0PID
        }
    }
    else {
        if (stall) {
            OTG.outEps[addr].DOEPCTL |= stallbit;
        }
        else {
            OTG.outEps[addr].DOEPCTL &= ~stallbit;
            OTG.outEps[addr].DOEPCTL |= (1 << 28);   // SD0PID
        }
    }
}

bool Stm32f10xOtg::epIsStalled(uint8_t addr)
{
    const uint32_t stallbit = (1 << 21);

    if (isInEp(addr))
        return (OTG.inEps[addr & 0x7f].DIEPCTL & stallbit) != 0;
    else
        return (OTG.outEps[addr].DOEPCTL & stallbit) != 0;
}

void Stm32f10xOtg::epSetNak(uint8_t addr, uint8_t nak)
{
    // n/a for IN endpoints
    if (isInEp(addr))
        return;

    forceNak[addr] = nak;
    // set or clear nak accordingly
    OTG.outEps[addr].DOEPCTL |= (nak ? (1 << 27) : (1 << 26));
}

uint16_t Stm32f10xOtg::epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    addr &= 0x7F;
    volatile USBOTG_IN_EP_t & ep = OTG.inEps[addr];

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
        uint32_t extra = *fifo++;
        memcpy(buf32, &extra, i);
    }

    // restore state
    volatile USBOTG_OUT_EP_t & ep = OTG.outEps[addr];
    ep.DOEPTSIZ = doeptsiz[addr];
    ep.DOEPCTL |= (1 << 31) |   // EPENA
                  (forceNak[addr] ? (1 << 27) : (1 << 26));

    return len;
}

void Stm32f10xOtg::isr()
{
    uint32_t status = OTG.global.GINTSTS;

    const uint32_t enumdne = 1 << 13;
    if (status & enumdne) {   //
        // Handle USB RESET condition.
        OTG.global.GINTSTS = enumdne;
        fifoMemTop = RX_FIFO_SIZE;
        Usbd::reset();
        return;
    }

    const uint32_t RXFLVL = 1 << 4;
    if (status & RXFLVL) {
        OTG.global.GINTSTS = RXFLVL;
        // Receive FIFO non-empty
        uint32_t rxstsp = OTG.global.GRXSTSP;
        uint32_t pktsts = rxstsp & (0xf << 17);  // PKTSTS mask

        uint8_t type;
        switch (pktsts) {
        case 0x6 << 17:
            type = TransactionSetup;
            break;
        case 0x2 << 17:
            type = TransactionOut;
            break;
        default:
            return;
        }

        uint8_t ep = rxstsp & 0xf; // EPNUM mask

        // Save packet size for epReadPacket()
        rxbcnt = (rxstsp & (0x7ff << 4)) >> 4;  // BCNT mask

        /*
         * FIXME: Why is a delay needed here?
         * This appears to fix a problem where the first 4 bytes
         * of the DATA OUT stage of a control transaction are lost.
         */
        for (unsigned i = 0; i < 1000; i++)
            asm("nop");

//        if (_usbd_device.user_callback_ctr[ep][type])
//            _usbd_device.user_callback_ctr[ep][type] (ep);

        // Discard unread packet data
        volatile uint32_t *buf = OTG.epFifos[ep];
        for (unsigned i = 0; i < rxbcnt; i += 4)
            (void)*buf;

        rxbcnt = 0;
    }

    // There is no global interrupt flag for tx complete - check the XFRC bit in each inEp
    const uint32_t xfrc = 0x1;
    for (unsigned i = 0; i < 4; i++) {
        volatile USBOTG_IN_EP_t & ep = OTG.inEps[i];
        if (ep.DIEPINT & xfrc) {     // DIEPINTX XFRC
//            if (_usbd_device.user_callback_ctr[i][TransactionIn]) {
//                _usbd_device.user_callback_ctr[i][TransactionIn](i);
//            }
            ep.DIEPINT = xfrc;
        }
    }

    const uint32_t usbsusp = 1 << 11;
    if (status & usbsusp) {
//        if (_usbd_device.user_callback_suspend) {
//            _usbd_device.user_callback_suspend();
//        }
        OTG.global.GINTSTS = usbsusp;
    }

    const uint32_t wkupint = 1 << 31;
    if (status & wkupint) {
//        if (_usbd_device.user_callback_resume) {
//            _usbd_device.user_callback_resume();
//        }
        OTG.global.GINTSTS = wkupint;
    }

    const uint32_t sof = 1 << 3;
    if (status & sof) {
//        if (_usbd_device.user_callback_sof) {
//            _usbd_device.user_callback_sof();
//        }
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
