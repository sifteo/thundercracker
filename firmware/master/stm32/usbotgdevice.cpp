#include "usbotgdevice.h"
#include <cstring>  // for NULL...is this defined within the SDK anywhere?
#include <sifteo/macros.h>

// static
UsbOtgDevice UsbOtgDevice::instance;

void UsbOtgDevice::init()
{
    usbcore.init();

    this->deviceStatus = UsbOtg::Default;
    this->address = 0;

    // Init ep structures
    UsbOtg::Endpoint *ep;
    for (int i = 0; i < UsbOtgCore::NUM_ENDPOINTS; ++i) {
        // IN eps
        ep = &inEps[i];
        ep->direction = UsbOtg::In;
        ep->num = i;
        ep->tx_fifo_num = i;
        // set as Control until ep is actvated
        ep->type = UsbOtg::Control;
        ep->maxpacket =  UsbOtgCore::MAX_PACKET_SIZE;
        ep->xfer_buff = 0;
        ep->xfer_len = 0;

        // OUT eps
        ep = &outEps[i];
        ep->direction = UsbOtg::Out;
        ep->num = i;
        ep->tx_fifo_num = i;
        // set as Control until ep is activated
        ep->type = UsbOtg::Control;
        ep->maxpacket = UsbOtgCore::MAX_PACKET_SIZE;
        ep->xfer_buff = 0;
        ep->xfer_len = 0;
    }

    usbcore.disableGlobalIsr();
    usbcore.reset();
    usbcore.setMode(UsbOtg::DeviceMode);
    usbcore.initDevMode();
    usbcore.enableGlobalIsr();

//    enable board specific interrupts here
}

void UsbOtgDevice::endpointTx(uint8_t ep_addr, uint8_t *pbuf, uint32_t buf_len)
{
    UsbOtg::Endpoint *ep = &inEps[ep_addr & 0x7F];
    // Setup and start the Transfer
    ep->direction = UsbOtg::In;
    ep->num = ep_addr & 0x7F;
    ep->xfer_buff = pbuf;
    ep->xfer_count = 0;
    ep->xfer_len  = buf_len;

    if (ep->num == 0) {
        usbcore.ep0StartXfer(ep);
//        USB_OTG_EP0StartXfer(pdev, ep);
    }
    else {
//        USB_OTG_EPStartXfer(pdev, ep);
    }
}

void UsbOtgDevice::openEndpoint(uint8_t ep_addr, uint16_t ep_mps, UsbOtg::EndpointType ep_type)
{
    UsbOtg::Endpoint *ep;

    if ((ep_addr & 0x80) == 0x80) {
        ep = &inEps[ep_addr & 0x7F];
    }
    else {
        ep = &outEps[ep_addr & 0x7F];
    }
    ep->num   = ep_addr & 0x7F;

    ep->direction = (0x80 & ep_addr) ? UsbOtg::In : UsbOtg::Out;
    ep->maxpacket = ep_mps;
    ep->type = ep_type;
    if (ep->direction == UsbOtg::In) {
        ep->tx_fifo_num = ep->num;  // Assign a Tx FIFO
    }
    // Set initial data PID.
    if (ep_type == UsbOtg::Bulk) {
        ep->data_pid_start = 0;
    }
//    USB_OTG_EPActivate(pdev , ep );
}

// bits that must be written to in order to clear interrupt status
#define GINTSTS_RC_W1_MASK ((1 << 31) | (1 << 30) | (1 << 29) | (1 << 28) | \
                            (1 << 21) | (1 << 20) | (1 << 15) | (1 << 14) | \
                            (1 << 13) | (1 << 12) | (1 << 11) | (1 << 10) | \
                            (1 << 3)  | (1 << 1));

/*
  Main entry point for any USB ISR when in device mode.
  Dispatch to appropriate subroutines - each isr subroutine
  is suffixed with _isr to clarify that it's part of this processing.
*/
void UsbOtgDevice::isr(uint32_t status)
{
    if (status & (1 << 18)) {   // OUT endpoint interrupt
        handleOutEp_isr();
//        retval |= DCD_HandleOutEP_ISR(pdev);
    }

    if (status & (1 << 19)) {   // IN endpoint interrupt
//        retval |= DCD_HandleInEP_ISR(pdev);
        handleInEp_isr();
    }

    if (status & (1 << 31)) {   // WKUPINT: Resume/remote wakeup detected interrupt
//        retval |= DCD_HandleResume_ISR(pdev);
        handleResume_isr();
    }

    if (status & (1 << 11)) {   // USBSUSP: USB suspend
//        retval |= DCD_HandleUSBSuspend_ISR(pdev);
        handleUSBSuspend_isr();
    }

    if (status & (1 << 3)) {    // SOF: Start of frame
//        retval |= DCD_HandleSof_ISR(pdev);
        deviceClass->startOfFrame(0);
    }

    if (status & (1 << 4)) {    // RXFLVL: Rx FIFO non-empty
//        retval |= DCD_HandleRxStatusQueueLevel_ISR(pdev);
    }

    if (status & (1 << 12)) {   // USBRST: USB reset
//        retval |= DCD_HandleUsbReset_ISR(pdev);
        handleUsbReset_isr();
    }

    if (status & (1 << 13)) {   // ENUMDNE: Enumeration done
//        retval |= DCD_HandleEnumDone_ISR(pdev);
        handleEnumDone_isr();
    }

    if (status & (1 << 20)) {   // IISOIXFR: Incomplete isochronous IN transfer
//        retval |= DCD_IsoINIncomplete_ISR(pdev);
    }

    if (status & (1 << 21)) {   // INCOMPISOOUT: Incomplete isochronous OUT transfer
//        retval |= DCD_IsoOUTIncomplete_ISR(pdev);
    }
#ifdef VBUS_SENSING_ENABLED
    if (status & (1 << 30)) {   // SRQINT: Session request/new session detected interrupt
        retval |= DCD_SessionRequest_ISR(pdev);
    }

    if (status & (1 << 2)) {    // OTGINT: OTG interrupt
        retval |= DCD_OTG_ISR(pdev);
    }
#endif

    USBOTG.global.GINTSTS = status & GINTSTS_RC_W1_MASK; // clear any serviced interrupts
}

// dispatched from main ISR
// an OUT endpoint has triggered an interrupt - find the source and handle it
void UsbOtgDevice::handleOutEp_isr()
{
    uint32_t epnum;
    uint32_t epint = (USBOTG.device.DAINT & USBOTG.device.DAINTMSK) >> 16;

    for (epnum = 0; epint; epnum++, epint >>= 1) {
        if (!(epint & 0x1)) {
            continue;
        }

        uint32_t epstatus = USBOTG.outEps[epnum].DOEPINT & USBOTG.device.DOEPMSK;
        if (epstatus & (1 << 0)) { // XFRC: Transfer completed interrupt
            // Inform upper layer: data ready
            deviceClass->dataOut(0, epnum);
        }

        if (epstatus & (1 << 3)) {  // STUP: SETUP phase done
            /* inform the upper layer that a setup packet is available */
            setupStage_isr();
            deviceClass->setup(0, 0);
        }
        USBOTG.outEps[epnum].DOEPINT = epstatus;    // clear interrupts
    }
}

void UsbOtgDevice::parseSetupRequest(UsbOtg::SetupRequest &req)
{
    req.bmRequest     = setup_packet[0];
    req.bRequest      = setup_packet[1];
    req.wValue        = UsbOtg::SWAPBYTE(setup_packet +  2);
    req.wIndex        = UsbOtg::SWAPBYTE(setup_packet +  4);
    req.wLength       = UsbOtg::SWAPBYTE(setup_packet +  6);

    inEps[0].ctl_data_len = req.wLength;
    ep0State = UsbOtg::EpSetup;
}

void UsbOtgDevice::setEndpointStall(uint8_t epnum)
{
    UsbOtg::Endpoint *ep;
    if ((0x80 & epnum) == 0x80) {
      ep = &inEps[epnum & 0x7F];
    }
    else {
      ep = &outEps[epnum];
    }

    ep->is_stall = 1;
    // TODO - why do these fields get reinitialized here?
    ep->num = epnum & 0x7F;
    ep->direction = ((epnum & 0x80) == 0x80) ? UsbOtg::In : UsbOtg::Out;

    usbcore.setEndpointStall(ep);
}

void UsbOtgDevice::clearEndpointStall(uint8_t epnum)
{
    UsbOtg::Endpoint *ep;
    if ((0x80 & epnum) == 0x80) {
        ep = &inEps[epnum & 0x7F];
    }
    else {
        ep = &outEps[epnum];
    }

    ep->is_stall = 0;
    ep->num   = epnum & 0x7F;
    ep->direction = ((epnum & 0x80) == 0x80) ? UsbOtg::In : UsbOtg::Out;

//    USB_OTG_EPClearStall(pdev , ep);
    usbcore.clearEndpointStall(ep);
}

void UsbOtgDevice::setupStage_isr()
{
    UsbOtg::SetupRequest req;
    parseSetupRequest(req);

    switch (req.bmRequest & 0x1F) {
    case UsbOtg::ReqRecipientDevice:
//        USBD_StdDevReq (pdev, &req);
        break;

    case UsbOtg::ReqRecipientInterface:
//        USBD_StdItfReq(pdev, &req);
        break;

    case UsbOtg::ReqRecipientEndpoint:
//        USBD_StdEPReq(pdev, &req);
        break;

    default:
        setEndpointStall(req.bmRequest & 0x80);
        break;
    }
}

static inline uint32_t inEpMaskedInt(uint8_t epnum)
{
    uint32_t emp = USBOTG.device.DIEPEMPMSK;
    uint32_t mask = USBOTG.device.DIEPMSK | (((emp >> epnum) & 0x1) << 7);
    return USBOTG.inEps[epnum].DIEPINT & mask;
}

void UsbOtgDevice::handleInEp_isr()
{
    uint32_t epnum;
    uint32_t epInterruptMask = (USBOTG.device.DAINT & USBOTG.device.DAINTMSK) & 0xFFFF;

    for (epnum = 0; epInterruptMask; epnum++, epInterruptMask >>= 1) {
        if (!(epInterruptMask & 1)) {
            continue;
        }

        uint32_t epInt = inEpMaskedInt(epnum);
        if (epInt & (1 << 0)) { // XFRC: transfer complete interrupt
            USBOTG.device.DIEPEMPMSK |= (1 << epnum); // unmask IN FIFO empty interrupt
            // TX COMPLETE
//            USBD_DCD_INT_fops->DataInStage(pdev , epnum);
            dataInStage_isr(epnum);
        }

//        if (epInt & (1 << 3)) { // TOC: timeout condition
//        }
//        if (epInt & (1 << 4)) { // ITTXFE: IN token received when TxFIFO is empty
//        }
//        if (epInt & (1 << 6)) { // INEPNE: IN endpoint NAK effective
//        }
//        if (epInt & (1 << 1)) { // EPDISD: Endpoint disabled interrupt
//        }
        if (epInt & (1 << 7)) { // TXFE: transmit FIFO empty (or half empty, depending on GAHBCFG)
//            DCD_WriteEmptyTxFifo(pdev , epnum);
        }

        // TODO: ST code writes 1's to this to clear ISRs but that doesn't seem right...
        USBOTG.inEps[epnum].DIEPINT = 0;    // clear interrupts
    }
}

void UsbOtgDevice::dataInStage_isr(uint8_t epnum)
{
    UsbOtg::Endpoint *ep;

    if (epnum == 0) {
        ep = &inEps[0];
        if (ep0State == UsbOtg::EpDataIn) { //USB_OTG_EP0_DATA_IN) {
            if (ep->rem_data_len > ep->maxpacket) {
                ep->rem_data_len -=  ep->maxpacket;
//                USBD_CtlContinueSendData (pdev,
//                                          ep->xfer_buff,
//                                          ep->rem_data_len);
            }
            else
            { /* last packet is MPS multiple, so send ZLP packet */
                if ((ep->total_data_len %  ep->maxpacket == 0) &&
                    (ep->total_data_len >= ep->maxpacket) &&
                    (ep->total_data_len <  ep->ctl_data_len))
                {
//                    USBD_CtlContinueSendData(pdev , NULL, 0);
                    ep->ctl_data_len = 0;
                }
                else {
                    if (deviceStatus == UsbOtg::Configured) {
                        deviceClass->ep0txSent(0);
                    }
//                    USBD_CtlReceiveStatus(pdev);
                }
            }
        }
    }
    else if (deviceStatus == UsbOtg::Configured) {
        deviceClass->dataIn(0, epnum);
    }
}

void UsbOtgDevice::handleResume_isr()
{
#if 0   // TODO: support this
    if (pdev->cfg.low_power) {
        USB_OTG_PCGCCTL_TypeDef  power;
        /* un-gate USB Core clock */
        power.d32 = USB_OTG_READ_REG32(&pdev->regs.PCGCCTL);
        power.b.gatehclk = 0;
        power.b.stoppclk = 0;
        USB_OTG_WRITE_REG32(pdev->regs.PCGCCTL, power.d32);
    }
#endif

    USBOTG.device.DCTL |= (1 << 0);  // RWUSIG: Clear the Remote Wake-up Signaling

    userCallbacks->DeviceResumed();
    deviceStatus = UsbOtg::Configured;
}

void UsbOtgDevice::handleUSBSuspend_isr()
{
    deviceStatus = UsbOtg::Suspended;
    userCallbacks->DeviceSuspended();

    (void)USBOTG.device.DSTS; // dummy read of DSTS - necessary?
//    dsts.d32 = USB_OTG_READ_REG32(&pdev->regs.DREGS->DSTS);

#if 0   // TODO: support low power
    if((pdev->cfg.low_power) && (dsts.b.suspsts == 1)) {
        USB_OTG_PCGCCTL_TypeDef  power;
        /*  switch-off the clocks */
        power.d32 = 0;
        power.b.stoppclk = 1;
        USB_OTG_MODIFY_REG32(pdev->regs.PCGCCTL, 0, power.d32);

        power.b.gatehclk = 1;
        USB_OTG_MODIFY_REG32(pdev->regs.PCGCCTL, 0, power.d32);

        /* Request to enter Sleep mode after exit from current ISR */
        SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
    }
#endif
}

/*

*/
void UsbOtgDevice::dataOutStage_isr(uint8_t epnum)
{
    UsbOtg::Endpoint *ep;

    if (epnum == 0) {
        ep = &outEps[0];
        if (this->ep0State == UsbOtg::EpDataOut) {
            if (ep->rem_data_len > ep->maxpacket) {
                ep->rem_data_len -=  ep->maxpacket;
                controlContinueRx(ep->xfer_buff, MIN(ep->rem_data_len ,ep->maxpacket));
            }
            else {
                if (this->deviceStatus == UsbOtg::Configured) {
                    deviceClass->ep0rxReady(0);
                }
                controlSendStatus();
            }
        }
    }
    else if (this->deviceStatus == UsbOtg::Configured) {
        deviceClass->dataOut(0, epnum);
    }
}

/*

*/
void UsbOtgDevice::handleUsbReset_isr()
{
//    USB_OTG_DAINT_TypeDef    daintmsk;
//    USB_OTG_DOEPMSK_TypeDef  doepmsk;
//    USB_OTG_DIEPMSK_TypeDef  diepmsk;
//    USB_OTG_DCFG_TypeDef     dcfg;
//    USB_OTG_DCTL_TypeDef     dctl;
//    USB_OTG_GINTSTS_TypeDef  gintsts;
//    uint32_t i;

//    dctl.d32 = 0;
//    daintmsk.d32 = 0;
//    doepmsk.d32 = 0;
//    diepmsk.d32 = 0;
//    dcfg.d32 = 0;
//    gintsts.d32 = 0;


//    dctl.b.rmtwkupsig = 1;
//    USB_OTG_MODIFY_REG32(&pdev->regs.DREGS->DCTL, dctl.d32, 0 );
    USBOTG.device.DCTL |= (1 << 0);  // RWUSIG: Clear the Remote Wake-up Signaling

    // Flush the Tx FIFO
//    USB_OTG_FlushTxFifo(pdev ,  0 );
    usbcore.flushTxFifo(0);

//    for (i = 0; i < pdev->cfg.dev_endpoints ; i++) {
    for (unsigned i = 0; i < UsbOtgCore::NUM_ENDPOINTS; i++) {
        USBOTG.inEps[i].DIEPINT = 0xFF;
        USBOTG.outEps[i].DOEPINT = 0xFF;
//        USB_OTG_WRITE_REG32( &pdev->regs.INEP_REGS[i]->DIEPINT, 0xFF);
//        USB_OTG_WRITE_REG32( &pdev->regs.OUTEP_REGS[i]->DOEPINT, 0xFF);
    }
//    USB_OTG_WRITE_REG32( &pdev->regs.DREGS->DAINT, 0xFFFFFFFF );
    USBOTG.device.DAINT = 0xFFFFFFFF;

//    daintmsk.ep.in = 1;
//    daintmsk.ep.out = 1;
//    USB_OTG_WRITE_REG32( &pdev->regs.DREGS->DAINTMSK, daintmsk.d32 );
    USBOTG.device.DAINTMSK = ((1 << 16) |   // OEPM: OUT endpoint interrupt mask bits
                              (1 << 0));    // IEPM: IN endpoint interrupt mask bits

//    doepmsk.b.setup = 1;
//    doepmsk.b.xfercompl = 1;
//    doepmsk.b.ahberr = 1;
//    doepmsk.b.epdisabled = 1;
//    USB_OTG_WRITE_REG32( &pdev->regs.DREGS->DOEPMSK, doepmsk.d32 );
    USBOTG.device.DOEPMSK = (1 << 3) |  // STUPM: SETUP phase done mask
                            (1 << 1) |  // EDPM: Endpoint disabled interrupt mask
                            (1 << 0);   // XFRCM: transfer complete interrupt mask

//    diepmsk.b.xfercompl = 1;
//    diepmsk.b.timeout = 1;
//    diepmsk.b.epdisabled = 1;
//    diepmsk.b.ahberr = 1;
//    diepmsk.b.intknepmis = 1;
//    USB_OTG_WRITE_REG32( &pdev->regs.DREGS->DIEPMSK, diepmsk.d32 );
    USBOTG.device.DIEPMSK = (1 << 5) |  // INEPNMM: in token rxed with EP mismatch
                            (1 << 3) |  // TOM: timeout condition mask
                            (1 << 1) |  // EPDM: endpoint disabled interrupt mask
                            (1 << 0);   // XFRCM: transfer complete interrupt mask

//    dcfg.d32 = USB_OTG_READ_REG32( &pdev->regs.DREGS->DCFG);
//    dcfg.b.devaddr = 0;
//    USB_OTG_WRITE_REG32( &pdev->regs.DREGS->DCFG, dcfg.d32);
    USBOTG.device.DCFG &= ~(0x7F << 4); // reset device address to 0


    /* setup EP0 to receive SETUP packets */
//    USB_OTG_EP0_OutStart(pdev);
    // TODO

    /*Reset internal state machine */
//    USBD_DCD_INT_fops->Reset(pdev);
    openEndpoint(0x00, UsbOtg::MaxEp0Size, UsbOtg::Control);    // Open EP0 OUT
    openEndpoint(0x80, UsbOtg::MaxEp0Size, UsbOtg::Control);    // Open EP0 IN

    deviceStatus = UsbOtg::Default;
    userCallbacks->DeviceReset(usbcore.speed());
}

void UsbOtgDevice::handleEnumDone_isr()
{
//    USB_OTG_GINTSTS_TypeDef  gintsts;
//    USB_OTG_GUSBCFG_TypeDef  gusbcfg;

//    USB_OTG_EP0Activate(pdev);
    usbcore.activateEp0();

    // we can only ever be full speed, not high, so no need for this

    /* Set USB turn-around time based on device speed and PHY interface. */
//    gusbcfg.d32 = USB_OTG_READ_REG32(&pdev->regs.GREGS->GUSBCFG);

    /* Full or High speed */
//    if ( USB_OTG_GetDeviceSpeed(pdev) == USB_SPEED_HIGH)
//    {
//        pdev->cfg.speed            = USB_OTG_SPEED_HIGH;
//        pdev->cfg.mps              = USB_OTG_HS_MAX_PACKET_SIZE ;
//        gusbcfg.b.usbtrdtim = 9;
//    }
//    else
//    {
//        pdev->cfg.speed            = USB_OTG_SPEED_FULL;
//        pdev->cfg.mps              = USB_OTG_FS_MAX_PACKET_SIZE ;
//        gusbcfg.b.usbtrdtim = 5;
//    }

//    USB_OTG_WRITE_REG32(&pdev->regs.GREGS->GUSBCFG, gusbcfg.d32);
    USBOTG.global.GUSBCFG |= (5 << 10); // set turnaround time to 5 for full speed
}


/***********************
  Request helpers
************************/

/*
  Main dispatcher for device requests
*/
void UsbOtgDevice::standardDeviceReq(UsbOtg::SetupRequest &req)
{
    switch (req.bRequest) {
    case UsbOtg::RequestGetDescriptor:      requestGetDescriptor(req); break;
    case UsbOtg::RequestSetAddress:         requestSetAddress(req); break;
    case UsbOtg::RequestSetConfiguration:   requestSetConfig(req); break;
    case UsbOtg::RequestGetConfiguration:   requestGetConfig(req); break;
    case UsbOtg::RequestGetStatus:          requestGetStatus(req); break;
    case UsbOtg::RequestSetFeature:         setFeature(req); break;
    case UsbOtg::RequestClearFeature:       clearFeature(req); break;
    default:
        controlError(req);
        break;
    }
}

void UsbOtgDevice::requestGetDescriptor(UsbOtg::SetupRequest &req)
{
    uint16_t len;
    uint8_t *pbuf;

    switch (req.wValue >> 8){
    case UsbOtg::DescriptorTypeDevice:
        pbuf = deviceClass->getConfigDescriptor(usbcore.speed(), &len);
//        pbuf = pdev->dev.usr_device->GetDeviceDescriptor(pdev->cfg.speed, &len);
        if ((req.wLength == 64) ||(deviceStatus == UsbOtg::Default)) {
            len = 8;
        }
        break;

    case UsbOtg::DescriptorTypeConfiguration:
        pbuf = deviceClass->getConfigDescriptor(usbcore.speed(), &len);
//        pbuf   = (uint8_t *)pdev->dev.class_cb->GetConfigDescriptor(pdev->cfg.speed, &len);
        pbuf[1] = UsbOtg::DescriptorTypeConfiguration;
//        pdev->dev.pConfig_descriptor = pbuf;
        break;

    case UsbOtg::DescriptorTypeString:
        switch ((uint8_t)(req.wValue)) {
        case UsbOtg::IndexLangIdString:
//            pbuf = pdev->dev.usr_device->GetLangIDStrDescriptor(pdev->cfg.speed, &len);
            break;

        case UsbOtg::IndexManufacturerString:
//            pbuf = pdev->dev.usr_device->GetManufacturerStrDescriptor(pdev->cfg.speed, &len);
            break;

        case UsbOtg::IndexProductString:
//            pbuf = pdev->dev.usr_device->GetProductStrDescriptor(pdev->cfg.speed, &len);
            break;

        case UsbOtg::IndexSerialNumString:
//            pbuf = pdev->dev.usr_device->GetSerialStrDescriptor(pdev->cfg.speed, &len);
            break;

        case UsbOtg::IndexConfigString:
//            pbuf = pdev->dev.usr_device->GetConfigurationStrDescriptor(pdev->cfg.speed, &len);
            break;

        case UsbOtg::IndexInterfaceString:
//            pbuf = pdev->dev.usr_device->GetInterfaceStrDescriptor(pdev->cfg.speed, &len);
            break;

        default:
#ifdef USB_SUPPORT_USER_STRING_DESC
            pbuf = pdev->dev.class_cb->GetUsrStrDescriptor(pdev->cfg.speed, (req->wValue) , &len);
            break;
#else
            controlError(req);
            return;
#endif
        }
        break;
    case UsbOtg::DescriptorTypeDeviceQualifier:
        controlError(req);
        return;

    case UsbOtg::DescriptorTypeOtherSpeedConfiguration:
        controlError(req);
        return;

    default:
        controlError(req);
        return;
    }

    if ((len != 0) && (req.wLength != 0)) {
        len = MIN(len , req.wLength);
        controlTx(pbuf, len);
    }
}

void UsbOtgDevice::requestSetAddress(UsbOtg::SetupRequest &req)
{
    uint8_t  dev_addr;

    if ((req.wIndex == 0) && (req.wLength == 0)) {
        dev_addr = (uint8_t)(req.wValue) & 0x7F;

        if (deviceStatus == UsbOtg::Configured) {
            controlError(req);
        }
        else  {
            this->address = dev_addr;
//            DCD_EP_SetAddress(pdev, dev_addr);
            controlSendStatus();
            if (dev_addr != 0)  {
                deviceStatus = UsbOtg::Addressed;
            }
            else  {
                deviceStatus = UsbOtg::Default;
            }
        }
    }
    else  {
        controlError(req);
    }
}

void UsbOtgDevice::requestGetStatus(UsbOtg::SetupRequest &req)
{
    switch (deviceStatus)  {
    case UsbOtg::Addressed:
    case UsbOtg::Configured:
//        if (pdev->dev.DevRemoteWakeup) {
//            USBD_cfg_status = USB_CONFIG_SELF_POWERED | USB_CONFIG_REMOTE_WAKEUP;
//        }
//        else {
//            USBD_cfg_status = USB_CONFIG_SELF_POWERED;
//        }
        controlTx((uint8_t *)&USBD_cfg_status, 1);
        break;

    default :
        controlError(req);
        break;
    }
}

void UsbOtgDevice::requestSetConfig(UsbOtg::SetupRequest &req)
{
    static uint8_t  cfgidx;

    cfgidx = (uint8_t)(req.wValue);

    if (cfgidx > UsbOtg::MaxConfigurationCount) {
        controlError(req);
    }
    else {
        switch (deviceStatus) {
        case UsbOtg::Addressed:
            if (cfgidx) {
                deviceConfig = cfgidx;
                deviceStatus = UsbOtg::Configured;
                //        USBD_SetCfg(pdev , cfgidx);
                controlSendStatus();
            }
            else {
                controlSendStatus();
            }
            break;

        case UsbOtg::Configured:
            if (cfgidx == 0) {
                deviceStatus = UsbOtg::Addressed;
                deviceConfig = cfgidx;
//                USBD_ClrCfg(pdev , cfgidx);
                controlSendStatus();
            }
            else  if (cfgidx != deviceConfig) {
//                /* Clear old configuration */
//                USBD_ClrCfg(pdev , deviceConfig);
//                /* set new configuration */
                deviceConfig = cfgidx;
//                USBD_SetCfg(pdev , cfgidx);
                controlSendStatus();
            }
            else {
                controlSendStatus();
            }
            break;

        default:
            controlError(req);
            break;
        }
    }
}

void UsbOtgDevice::requestGetConfig(UsbOtg::SetupRequest &req)
{
  if (req.wLength != 1) {
      controlError(req);
  }
  else {
    switch (deviceStatus) {
    case UsbOtg::Addressed:
      controlTx((uint8_t *)&USBD_default_cfg, 1);
      break;

    case UsbOtg::Configured:
      controlTx(&deviceConfig, 1);
      break;

    default:
       controlError(req);
      break;
    }
  }
}

void UsbOtgDevice::controlError(UsbOtg::SetupRequest &req)
{
    if ((req.bmRequest & 0x80) == 0x80) {
        setEndpointStall(0x80);
    }
    else {
        if (req.wLength == 0) {
            setEndpointStall(0x80);
        }
        else {
            setEndpointStall(0x0);
        }
    }
//    USB_OTG_EP0_OutStart(pdev);
}

void UsbOtgDevice::setFeature(UsbOtg::SetupRequest &req)
{
//    USB_OTG_DCTL_TypeDef     dctl;
//    uint8_t test_mode = 0;

    if (req.wValue == UsbOtg::FeatureRemoteWakeup) {
//        pdev->dev.DevRemoteWakeup = 1;
//        pdev->dev.class_cb->Setup (pdev, req);
        deviceClass->setup(0, &req);
        controlSendStatus();
    }
#if 0
    else if ((req.wValue == USB_FEATURE_TEST_MODE) && ((req.wIndex & 0xFF) == 0))
    {
        dctl.d32 = USB_OTG_READ_REG32(&pdev->regs.DREGS->DCTL);

        test_mode = req.wIndex >> 8;
        switch (test_mode) {
        case 1: // TEST_J
            dctl.b.tstctl = 1;
            break;

        case 2: // TEST_K
            dctl.b.tstctl = 2;
            break;

        case 3: // TEST_SE0_NAK
            dctl.b.tstctl = 3;
            break;

        case 4: // TEST_PACKET
            dctl.b.tstctl = 4;
            break;

        case 5: // TEST_FORCE_ENABLE
            dctl.b.tstctl = 5;
            break;
        }
        USB_OTG_WRITE_REG32(&pdev->regs.DREGS->DCTL, dctl.d32);
        USBD_CtlSendStatus(pdev);
    }
#endif
}

void UsbOtgDevice::clearFeature(UsbOtg::SetupRequest &req)
{
    switch (deviceStatus) {
    case UsbOtg::Addressed:
    case UsbOtg::Configured:
        if (req.wValue == UsbOtg::FeatureRemoteWakeup) {
//            pdev->dev.DevRemoteWakeup = 0;    // TODO - what is this?
            deviceClass->setup(0, &req);
            controlSendStatus();
        }
        break;

    default :
        controlError(req);
        break;
    }
}

void UsbOtgDevice::standardInterfaceReq(UsbOtg::SetupRequest &req)
{
    switch (deviceStatus) {
    case UsbOtg::Configured:
        if (UsbOtg::LOBYTE(req.wIndex) <= UsbOtg::MaxInterfaceCount) {
            deviceClass->setup(0, &req);    // TODO - check return value here?
            if((req.wLength == 0)) {
                controlSendStatus();
            }
        }
        else {
            controlError(req);
        }
        break;

    default:
        controlError(req);
        break;
    }
}

void UsbOtgDevice::standardEndpointReq(UsbOtg::SetupRequest &req)
{
    uint8_t ep_addr = UsbOtg::LOBYTE(req.wIndex);

    switch (req.bRequest) {
    case UsbOtg::RequestSetFeature:
        switch (deviceStatus) {
        case UsbOtg::Addressed:
            if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
                setEndpointStall(ep_addr);
            }
            break;

        case UsbOtg::Configured:
            if (req.wValue == UsbOtg::FeatureEpHalt) {
                if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
                    setEndpointStall(ep_addr);
                }
            }
            deviceClass->setup(0, &req);
            controlSendStatus();
            break;

        default:
            controlError(req);
            break;
        }
        break;  // UsbOtg::RequestSetFeature

    case UsbOtg::RequestClearFeature:
        switch (deviceStatus) {
        case UsbOtg::Addressed:
            if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
                setEndpointStall(ep_addr);
            }
            break;

        case UsbOtg::Configured:
            if (req.wValue == UsbOtg::FeatureEpHalt) {
                if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
                    clearEndpointStall(ep_addr);
                    deviceClass->setup(0, &req);
                }
                controlSendStatus();
            }
            break;

        default:
            controlError(req);
            break;
        }
        break;  // UsbOtg::RequestClearFeature

    case UsbOtg::RequestGetStatus:
        switch (deviceStatus) {
        case UsbOtg::Addressed:
            if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
                setEndpointStall(ep_addr);
            }
            break;

        case UsbOtg::Configured:
            if ((ep_addr & 0x80)== 0x80) {
                if(inEps[ep_addr & 0x7F].is_stall) {
                    USBD_ep_status = 0x0001;
                }
                else {
                    USBD_ep_status = 0x0000;
                }
            }
            else if ((ep_addr & 0x80)== 0x00) {
                if (outEps[ep_addr].is_stall) {
                    USBD_ep_status = 0x0001;
                }
                else {
                    USBD_ep_status = 0x0000;
                }
            }
            controlTx((uint8_t *)&USBD_ep_status, 2);
            break;

        default:
            controlError(req);
            break;
        }
        break;  // UsbOtg::RequestGetStatus

    default:
        break;
    }
}



/*******************************************************
    Control Helpers
********************************************************/

/*
    send zero length packet on the ctl pipe
*/
void UsbOtgDevice::controlSendStatus()
{
    ep0State = UsbOtg::EpStatusIn;
    endpointTx(0, NULL, 0);
//    USB_OTG_EP0_OutStart(pdev);
}

/*
    receive zero length packet on the ctl pipe
*/
void UsbOtgDevice::controlReceiveStatus()
{
    ep0State = UsbOtg::EpStatusOut;
//  pdev->dev.device_state = USB_OTG_EP0_STATUS_OUT;
//    DCD_EP_PrepareRx ( pdev, 0, NULL, 0);
//    USB_OTG_EP0_OutStart(pdev);
}

void UsbOtgDevice::controlPrepareRx(uint8_t *pbuf, uint16_t len)
{
    outEps[0].total_data_len = len;
    outEps[0].rem_data_len   = len;
    ep0State = UsbOtg::EpDataOut;
    //  DCD_EP_PrepareRx (pdev, 0, pbuf, len);
}

/*
    continue receive data on the ctl pipe
*/
void UsbOtgDevice::controlContinueRx(uint8_t *pbuf, uint16_t len)
{
//    DCD_EP_PrepareRx (pdev, 0, pbuf, len);
}

void UsbOtgDevice::controlContinueTx(uint8_t *pbuf, uint16_t len)
{
    endpointTx(0, pbuf, len);
}

void UsbOtgDevice::controlTx(uint8_t *pbuf, uint16_t len)
{
    inEps[0].total_data_len = len;
    inEps[0].rem_data_len   = len;
    ep0State = UsbOtg::EpDataIn;
    endpointTx(0, pbuf, len);
}
