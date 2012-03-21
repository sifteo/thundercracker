/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usbotgcore.h"
#include "usbotgdevice.h"

// static
UsbOtgCore UsbOtgCore::instance(GPIOPin(&GPIOA, 8),     // sof
                                GPIOPin(&GPIOA, 9),     // vbus
                                GPIOPin(&GPIOA, 11),    // dm
                                GPIOPin(&GPIOA, 12));   // dp

void UsbOtgCore::init()
{
    RCC.AHBENR |= (1 << 12); // OTGFSEN: USB OTG enabl
    // TODO - configure GPIOs

    USBOTG.global.GCCFG =   (1 << 20) | // SOFOUTEN: SOF pulse available on PAD
                            (1 << 19) | // VBUSBSEN: Enable the VBUS sensing “B” device
                            (1 << 18) | // VBUSASEN: Enable the VBUS sensing “A” device
                            (1 << 16);  // PWRDWN: Power down deactivated (“Transceiver active”)
//    USB_OTG_BSP_mDelay(20);
    USBOTG.global.GUSBCFG |=    (1 << 9) |  // HNPCAP: HNP-capable
                                (1 << 8);   // SRPCAP: SRP-capable
    enableCommonInterrupts();
}

void UsbOtgCore::reset()
{
    // AHB master must be idle before reset
    while (USBOTG.global.GRSTCTL & (1 << 31)) // AHBIDL: AHB master idle
        ;
    // issue the soft reset & wait for it to complete
    USBOTG.global.GRSTCTL = (1 << 0);   // CSRST: Core soft reset
    while (USBOTG.global.GRSTCTL & (1 << 0))
        ;
    // TODO - wait for 3 PHY cycles
}

void UsbOtgCore::stop()
{
    RCC.AHBENR &= ~(1 << 12); // OTGFSEN: USB enable
}

void UsbOtgCore::enableCommonInterrupts()
{
#ifdef USE_OTG_MODE
      USBOTG.global.GOTGINT = 0xFFFFFFFF;   // clear pending otg interrupts
#endif
      USBOTG.global.GINTSTS = 0xFFFFFFFF;   // clear pending interrupts
      USBOTG.global.GINTMSK =
#ifdef USE_OTG_MODE
            (1 << 2) |  // OTGINT: OTG interrupt
            (1 << 30) | // SRQINT: Session request/new session detected interrupt
            (1 << 28) | // CIDSCHG: Connector ID status change
#endif
            (1 << 31) | // WKUPINT: Resume/remote wakeup detected interrupt
            (1 << 11); // USBSUSP: USB suspend
}

void UsbOtgCore::enableDeviceInterrupts()
{
    USBOTG.global.GINTMSK = 0;              // Disable all interrupts.
    USBOTG.global.GINTSTS = 0xFFFFFFFF;     // Clear any pending interrupts
    enableCommonInterrupts();
    // Enable interrupts matching to the Device mode ONLY
    USBOTG.global.GINTMSK |=
#ifdef VBUS_SENSING_ENABLED
            (1 << 30) |     // SRQIM: Session request/new session detected interrupt mask
            (1 << 2) |      // OTGINT: OTG interrupt mask
#endif
            (1 << 21) |     // IISOOXFRM: Incomplete isochronous OUT transfer mask
            (1 << 20) |     // IISOIXFRM: Incomplete isochronous IN transfer mask
            (1 << 19) |     // OEPINT: out endpoint interrupt
            (1 << 18) |     // IEPINT: in endpoint interrupt
            (1 << 13) |     // ENUMDNEM: Enumeration done mask
            (1 << 12) |     // USBRST: USB reset mask
            (1 << 11) |     // USBSUSPM: USB suspend mask
            (1 << 3);       // SOFM: Start of frame mask
}

void UsbOtgCore::setMode(UsbOtg::Mode mode)
{
    uint32_t usbcfg = USBOTG.global.GUSBCFG;
    usbcfg &= ~(UsbOtg::HostMode | UsbOtg::DeviceMode); // clear FDMOD & FHMOD
    USBOTG.global.GUSBCFG = usbcfg | mode;
    // takes at least 25 ms for mode change to take effect
    // USB_OTG_BSP_mDelay(50);
}

// app-specific config - find a better place to put these
#define RX_FIFO_FS_SIZE     128
#define TX1_FIFO_FS_SIZE    64
#define TX2_FIFO_FS_SIZE    64
#define TX3_FIFO_FS_SIZE    64

void UsbOtgCore::initDevMode()
{
    USBOTG.PCGCCTL = 0; // Restart the PHY Clock
    USBOTG.device.DCFG |= UsbOtg::FrameInterval80 |
                            (3 << 0);   // DSPD: full speed
    USBOTG.global.GRXFSIZ = RX_FIFO_FS_SIZE; // set Rx FIFO size

    uint16_t startAddr = RX_FIFO_FS_SIZE;
    uint16_t fifoDepth = RX_FIFO_FS_SIZE;

    // EP0 TX
    USBOTG.global.DIEPTXF0_HNPTXFSIZ =
            (startAddr << 16) |   // NPTXFD: Non-periodic TxFIFO depth (in terms of 32-bit words)
            (fifoDepth << 0);     // NPTXFSA: Non-periodic transmit RAM start address

    startAddr += fifoDepth; // EP1 TX
    fifoDepth = TX1_FIFO_FS_SIZE;
    USBOTG.global.DIEPTXF[0] = (startAddr << 16 | fifoDepth);

    startAddr += fifoDepth; // EP2 TX
    fifoDepth = TX2_FIFO_FS_SIZE;
    USBOTG.global.DIEPTXF[1] = (startAddr << 16 | fifoDepth);

    startAddr += fifoDepth; // EP3 TX
    fifoDepth = TX3_FIFO_FS_SIZE;
    USBOTG.global.DIEPTXF[2] = (startAddr << 16 | fifoDepth);

    // clear all fifos
    flushTxFifo(0x10); // 0x10 means 'all tx fifos'
    flushRxFifo();

    // Clear all pending Device Interrupts
    USBOTG.device.DIEPMSK = 0;
    USBOTG.device.DOEPMSK = 0;
    USBOTG.device.DAINT = 0xFFFFFFFF;
    USBOTG.device.DAINTMSK = 0;

    // ensure all endpoints are disabled.
    // can only set disabled bit if they're already enabled
    for (unsigned i = 0; i < NUM_ENDPOINTS; i++) {
        // first IN endpoint
        uint32_t iepctl = USBOTG.inEps[i].DIEPCTL;
        if (iepctl & (1 << 31)) {               // if enabled
            iepctl |= ((1 << 30) | (1 << 27));  // set disabled and NAK bits
        }
        else {
            iepctl = 0;
        }
        USBOTG.inEps[i].DIEPCTL = iepctl;
        USBOTG.inEps[i].DIEPTSIZ = 0;
        USBOTG.inEps[i].DIEPINT = 0xFF;

        // and same for OUT endpoint
        uint32_t oepctl = USBOTG.outEps[i].DOEPCTL;
        if (oepctl & (1 << 31)) {
            oepctl |= ((1 << 30) | (1 << 27));  // set disabled and NAK bits
        }
        else {
            oepctl = 0;
        }
        USBOTG.outEps[i].DOEPCTL = oepctl;
        USBOTG.outEps[i].DOEPTSIZ = 0;
        USBOTG.outEps[i].DOEPINT = 0xFF;
    }

    // TODO - not clear what this chunk was actually doing...
//    msk.d32 = 0;
//    msk.b.txfifoundrn = 1;
//    USBOTG.device.DIEPMSK, msk.d32, msk.d32);

    enableDeviceInterrupts();
}

void UsbOtgCore::flushTxFifo(uint8_t num)
{
    // ensure the core is not writing anything to the fifo
    while (USBOTG.global.GRSTCTL & (1 << 31)) // AHBIDL: AHB master idle
        ;
    USBOTG.global.GRSTCTL = (1 << 5) |  // TXFFLSH: Tx FIFO flush
                            (num << 6); // TXFNUM: Tx FIFO number, 0x10 means ALL fifos
    while (USBOTG.global.GRSTCTL & (1 << 5))    // wait for tx flush complete
        ;
  // Wait for 3 PHY Clocks
//    USB_OTG_BSP_uDelay(3);
}

void UsbOtgCore::flushRxFifo()
{
    USBOTG.global.GRSTCTL = (1 << 4);           // RXFFLSH: rx FIFO flush
    while (USBOTG.global.GRSTCTL & (1 << 4))    // wait for rx flush complete
        ;
    // Wait for 3 PHY Clocks
//    USB_OTG_BSP_uDelay(3);
}

void UsbOtgCore::ep0StartXfer(UsbOtg::Endpoint *ep)
{
#if 0
    USB_OTG_DEPCTL_TypeDef      depctl;
    USB_OTG_DEP0XFRSIZ_TypeDef  deptsiz;
    USB_OTG_INEPREGS          *in_regs;
    uint32_t fifoemptymsk = 0;

    depctl.d32   = 0;
    deptsiz.d32  = 0;
    // IN endpoint
    if (ep->is_in == 1) {
        in_regs = pdev->regs.INEP_REGS[0];
        depctl.d32  = USB_OTG_READ_REG32(&in_regs->DIEPCTL);
        deptsiz.d32 = USB_OTG_READ_REG32(&in_regs->DIEPTSIZ);
        // Zero Length Packet?
        if (ep->xfer_len == 0) {
            deptsiz.b.xfersize = 0;
            deptsiz.b.pktcnt = 1;
        }
        else {
            if (ep->xfer_len > ep->maxpacket) {
                ep->xfer_len = ep->maxpacket;
                deptsiz.b.xfersize = ep->maxpacket;
            }
            else {
                deptsiz.b.xfersize = ep->xfer_len;
            }
            deptsiz.b.pktcnt = 1;
        }
        USB_OTG_WRITE_REG32(&in_regs->DIEPTSIZ, deptsiz.d32);

        // EP enable, IN data in FIFO
        depctl.b.cnak = 1;
        depctl.b.epena = 1;
        USB_OTG_WRITE_REG32(&in_regs->DIEPCTL, depctl.d32);

        /* Enable the Tx FIFO Empty Interrupt for this EP */
        if (ep->xfer_len > 0) {
            fifoemptymsk |= 1 << ep->num;
            USB_OTG_MODIFY_REG32(&pdev->regs.DREGS->DIEPEMPMSK, 0, fifoemptymsk);
        }
    }
    else {
        // OUT endpoint
        depctl.d32  = USB_OTG_READ_REG32(&pdev->regs.OUTEP_REGS[ep->num]->DOEPCTL);
        deptsiz.d32 = USB_OTG_READ_REG32(&pdev->regs.OUTEP_REGS[ep->num]->DOEPTSIZ);
        /* Program the transfer size and packet count as follows:
    * xfersize = N * (maxpacket + 4 - (maxpacket % 4))
    * pktcnt = N           */
        if (ep->xfer_len == 0) {
            deptsiz.b.xfersize = ep->maxpacket;
            deptsiz.b.pktcnt = 1;
        }
        else {
            ep->xfer_len = ep->maxpacket;
            deptsiz.b.xfersize = ep->maxpacket;
            deptsiz.b.pktcnt = 1;
        }
        USB_OTG_WRITE_REG32(&pdev->regs.OUTEP_REGS[ep->num]->DOEPTSIZ, deptsiz.d32);
        // EP enable
        depctl.b.cnak = 1;
        depctl.b.epena = 1;
        USB_OTG_WRITE_REG32 (&(pdev->regs.OUTEP_REGS[ep->num]->DOEPCTL), depctl.d32);
    }
#endif
}

void UsbOtgCore::setEndpointStall(UsbOtg::Endpoint *ep)
{
    if (ep->direction == UsbOtg::In) {
        uint32_t epCtl = USBOTG.inEps[ep->num].DIEPCTL;
        if (epCtl & (1 << 31)) {    // EPENA: endpoint enabled?
            epCtl |= (1 << 30);     // EPDIS: disable it
        }
        epCtl |= (1 << 21);         // set the stall bit
        USBOTG.inEps[ep->num].DIEPCTL = epCtl;
    }
    else {
        USBOTG.outEps[ep->num].DOEPCTL |= (1 << 21); // set the stall bit
    }
}

void UsbOtgCore::clearEndpointStall(UsbOtg::Endpoint *ep)
{
    volatile uint32_t *epCtlAddress;
    uint32_t epCtl;

    if (ep->direction == UsbOtg::In) {
        epCtlAddress = &USBOTG.inEps[ep->num].DIEPCTL;
    }
    else {
        epCtlAddress = &USBOTG.outEps[ep->num].DOEPCTL;
    }
    epCtl = *epCtlAddress;
    epCtl &= ~(1 << 21); // clear the stall bit
    if (ep->type == UsbOtg::Interrupt || ep->type == UsbOtg::Bulk) {
        epCtl |= (1 << 28); // set DATA0 PID
    }
    *epCtlAddress = epCtl;
}

/*
    Enable EP0 OUT to receive SETUP packets and configure EP0
    for transmitting packets
*/
void UsbOtgCore::activateEp0()
{
//    USB_OTG_DSTS_TypeDef    dsts;
//    USB_OTG_DEPCTL_TypeDef  diepctl;
//    USB_OTG_DCTL_TypeDef    dctl;

//    dctl.d32 = 0;
    // Read the Device Status and Endpoint 0 Control registers

    // Set the max packet size of the IN EP based on the enumeration speed
    uint8_t enumSpeed = (USBOTG.device.DSTS >> 1) & 0x3;
    switch (enumSpeed) {
    case UsbOtg::HighSpeedPhy30_or_60Mhz:
    case UsbOtg::FullSpeedPhy30_or_60Mhz:
    case UsbOtg::FullSpeedPhy48Mhz:
//        diepctl.b.mps = DEP0CTL_MPS_64;
        USBOTG.inEps[0].DIEPCTL &= ~(0x3);  // 0 == 64 bytes
        break;
    case UsbOtg::LowSpeedPhy6Mhz:
//        diepctl.b.mps = DEP0CTL_MPS_8;
        USBOTG.inEps[0].DIEPCTL |= 0x3;     // 0x3 == 8 bytes
        break;
    }

//    dsts.d32 = USB_OTG_READ_REG32(&pdev->regs.DREGS->DSTS);
//    diepctl.d32 = USB_OTG_READ_REG32(&pdev->regs.INEP_REGS[0]->DIEPCTL);
//    // Set the MPS of the IN EP based on the enumeration speed
//    switch (dsts.b.enumspd) {
//    case HighSpeedPhy30_or_60Mhz:
//    case FullSpeedPhy30_or_60Mhz:
//    case FullSpeedPhy48Mhz:
//        diepctl.b.mps = DEP0CTL_MPS_64;
//        break;
//    case LowSpeedPhy6Mhz:
//        diepctl.b.mps = DEP0CTL_MPS_8;
//        break;
//    }
//    USB_OTG_WRITE_REG32(&pdev->regs.INEP_REGS[0]->DIEPCTL, diepctl.d32);
//    dctl.b.cgnpinnak = 1;
//    USB_OTG_MODIFY_REG32(&pdev->regs.DREGS->DCTL, dctl.d32, dctl.d32);

    USBOTG.device.DCTL |= (1 << 8); // clear global IN NAK
}

void UsbOtgCore::startEndpointXfer(UsbOtg::Endpoint *ep)
{
    uint32_t epCtl, epSize;
    USB_OTG_DEPCTL_TypeDef     depctl;
    USB_OTG_DEPXFRSIZ_TypeDef  deptsiz;

    depctl.d32 = 0;
    deptsiz.d32 = 0;

    if (ep->direction == UsbOtg::In) {
        // Zero Length Packet?
        if (ep->xfer_len == 0) {
            epSize = (1 << 19); // set packet count to 1 and xfer size to 0
        }
        else {
            //  Program the transfer size and packet count as follows:
            //  xfersize = N * maxpacket + short_packet
            //  pktcnt = N + (short_packet exist ? 1 : 0)
            uint16_t pkcnt = (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;

            epSize =    ((ep->type == UsbOtg::Isochronous ? 1 : 0) << 29) |
                        (pkcnt << 19) |
                        (ep->xfer_len);
        }
        USBOTG.inEps[ep->num].DIEPTSIZ = epSize;

        epCtl = USBOTG.inEps[ep->num].DIEPCTL;
        if (ep->type == UsbOtg::Isochronous) {
            uint32_t sts = USBOTG.device.DSTS;

            // toggle the packet ID based on frame number of the received SOF
            if ((sts >> 8) & 0x1) { // odd or even?
                epCtl |= (1 << 28);
            }
            else {
                epCtl |= (1 << 29);
            }
        }
        else {
            // Enable the Tx FIFO Empty Interrupt
            if (ep->xfer_len > 0) {
                uint32_t fifoemptymsk = 1 << ep->num;
                USBOTG.device.DIEPEMPMSK |= fifoemptymsk;
            }
        }

        // EP enable, IN data in FIFO
        USBOTG.inEps[ep->num].DIEPCTL = epCtl |
                                        (1 << 26) | // CNAK: clear any naks
                                        (1 << 31);  // EPENA: endpoint enabled
        if (ep->type == UsbOtg::Isochronous) {
            writePacket(ep->xfer_buff, ep->num, ep->xfer_len);
        }
    }
    else {  // OUT endpoint
        epCtl = USBOTG.outEps[ep->num].DOEPCTL;
//        epSize = USBOTG.outEps[ep->num].DOEPTSIZ;
        /*
            Program the transfer size and packet count as follows:
            pktcnt = N
            xfersize = N * maxpacket
        */
        if (ep->xfer_len == 0) {
//            deptsiz.b.xfersize = ep->maxpacket;
//            deptsiz.b.pktcnt = 1;
            epSize = (1 << 19); // set packet count to 1 and xfer size to 0
        }
        else {
            uint16_t pktcnt = (ep->xfer_len + (ep->maxpacket - 1)) / ep->maxpacket;
            uint16_t xfersize = deptsiz.b.pktcnt * ep->maxpacket;
            epSize = (pktcnt << 19) | xfersize;
        }
        USBOTG.outEps[ep->num].DOEPTSIZ = epSize;

        if (ep->type == UsbOtg::Isochronous) {
            if (ep->even_odd_frame) {
                depctl.b.setd1pid = 1;
            }
            else {
                depctl.b.setd0pid = 1;
            }
        }
        // EP enable
        depctl.b.cnak = 1;
        depctl.b.epena = 1;
        USB_OTG_WRITE_REG32(&pdev->regs.OUTEP_REGS[ep->num]->DOEPCTL, depctl.d32);
    }
}

void UsbOtgCore::writePacket(uint8_t *src, uint8_t ch_ep_num, uint16_t len)
{
    uint32_t count32b =  (len + 3) / sizeof(uint32_t);
    volatile uint32_t *fifo = USBOTG.DFIFO[ch_ep_num];
    uint32_t *srcp = (uint32_t*)src;

//    fifo = pdev->regs.DFIFO[ch_ep_num];
//    for (i = 0; i < count32b; i++, src += 4) {
    while (count32b--) {
//        USB_OTG_WRITE_REG32( fifo, *((__packed uint32_t *)src) );
        *fifo = *srcp++;
    }
}

void* UsbOtgCore::readPacket(uint8_t *dest, uint16_t len)
{
    uint32_t count32b = (len + 3) / sizeof(uint32_t);
    uint32_t *destp = (uint32_t*)dest;

//    volatile uint32_t *fifo = pdev->regs.DFIFO[0];
    volatile uint32_t *fifo = USBOTG.DFIFO[0];

//    for (i = 0; i < count32b; i++, dest += 4 ) {
    while (count32b--) {
        *destp++ =  *fifo;
//        *(__packed uint32_t *)dest = USB_OTG_READ_REG32(fifo);
    }
    return ((void *)dest);
}

void UsbOtgCore::isr()
{
    uint32_t status = USBOTG.global.GINTSTS & USBOTG.global.GINTMSK;
    if (status == 0) {  // spurious interrupt ?
        return;
    }

    // check mode for this interrupt and hand off to either host or device handler
    if (!(status & 0x1)) {
        UsbOtgDevice::instance.isr(status);
    }
    else {
        // host mode, someday?
    }
}
