#include "usbcontrol.h"
#include "usb/usbd.h"
#include "usb/usbhardware.h"
#include "usb/usbdriver.h"
#include "usb/usbcore.h"

#include "macros.h"

using namespace Usb;

UsbControl::ControlState UsbControl::controlState;

void UsbControl::sendChunk()
{
    const DeviceDescriptor *dd = Usbd::devDescriptor();
    if (dd->bMaxPacketSize0 < controlState.len) {
        // Data stage, normal transmission
        UsbHardware::epWritePacket(0, controlState.buf, dd->bMaxPacketSize0);
        controlState.status = DataIn;
        controlState.buf += dd->bMaxPacketSize0;
        controlState.len -= dd->bMaxPacketSize0;
    }
    else {
        // Data stage, end of transmission
        UsbHardware::epWritePacket(0, controlState.buf, controlState.len);
        controlState.status = LastDataIn;
        controlState.len = 0;
        controlState.buf = 0;
    }
}

int UsbControl::receiveChunk()
{
    uint16_t packetsize = MIN(Usbd::devDescriptor()->bMaxPacketSize0, controlState.req.wLength - controlState.len);
    uint16_t size = UsbHardware::epReadPacket(0, controlState.buf + controlState.len, packetsize);

    if (size != packetsize) {
        UsbHardware::epSetStalled(0, true);
        return -1;
    }

    controlState.len += size;

    return packetsize;
}

int UsbControl::requestDispatch(SetupData *req)
{

    int result = UsbDriver::controlRequest(req, &controlState.buf, &controlState.len);
    if (result)
        return result;

    return UsbCore::standardRequest(req, &controlState.buf, &controlState.len);
}

void UsbControl::setupRead(SetupData *req)
{
    controlState.buf = Usbd::ctrlBuf;
    controlState.len = req->wLength;

    if (requestDispatch(req)) {
        if (controlState.len) {
            // Go to data out stage if handled
            sendChunk();
        }
        else {
            // Go to status stage if handled
            UsbHardware::epWritePacket(0, 0, 0);
            controlState.status = StatusIn;
        }
    }
    else {
        UsbHardware::epSetStalled(0, true);    // stall on failure
    }
}

void UsbControl::setupWrite(SetupData *req)
{
    if (req->wLength > Usbd::ctrlBufLen) {
        UsbHardware::epSetStalled(0, true);
        return;
    }

    // Buffer into which to write received data
    controlState.buf = Usbd::ctrlBuf;
    controlState.len = 0;

    // Wait for DATA OUT stage
    if (req->wLength > Usbd::devDescriptor()->bMaxPacketSize0)
        controlState.status = DataOut;
    else
        controlState.status = LastDataOut;
}

bool UsbControl::controlRequest(uint8_t ep, Usb::Transaction txn)
{
    switch (txn) {
    case TransactionIn:
        in(ep);
        break;
    case TransactionOut:
        out(ep);
        break;
    case TransactionSetup:
        setup(ep);
        break;
    default:
        return false;
    }
    return true;
}

void UsbControl::setup(uint8_t ea)
{
    SetupData *req = &controlState.req;
    (void)ea;

    controlState.complete = NULL;

    if (UsbHardware::epReadPacket(0, req, 8) != 8) {
        UsbHardware::epSetStalled(0, true);
        return;
    }

    if (req->wLength == 0)
        setupRead(req);
    else if (req->bmRequestType & 0x80)
        setupRead(req);
    else
        setupWrite(req);
}

void UsbControl::out(uint8_t ea)
{
    (void)ea;

    switch (controlState.status) {
    case DataOut:
        if (receiveChunk() < 0)
            break;
        if ((controlState.req.wLength - controlState.len) <= Usbd::devDescriptor()->bMaxPacketSize0)
            controlState.status = LastDataOut;
        break;

    case LastDataOut:
        if (receiveChunk() < 0)
            break;

        // We have now received the full data payload.
        // Invoke callback to process.
        if (requestDispatch(&controlState.req)) {
            // Got to status stage on success
            UsbHardware::epWritePacket(0, NULL, 0);
            controlState.status = StatusIn;
        }
        else {
            UsbHardware::epSetStalled(0, true);
        }
        break;

    case StatusOut:
        UsbHardware::epReadPacket(0, NULL, 0);
        controlState.status = Idle;
        if (controlState.complete) {
            controlState.complete(&controlState.req);
        }
        controlState.complete = NULL;
        break;

    default:
        UsbHardware::epSetStalled(0, true);
    }
}

void UsbControl::in(uint8_t ea)
{
    (void)ea;
    SetupData &req = controlState.req;

    switch (controlState.status) {
    case DataIn:
        sendChunk();
        break;

    case LastDataIn:
        controlState.status = StatusOut;
        break;

    case StatusIn:
        if (controlState.complete)
            controlState.complete(&controlState.req);

        // Exception: Handle SET ADDRESS function here...
        if ((req.bmRequestType == 0) && (req.bRequest == RequestSetAddress))
            UsbHardware::setAddress(req.wValue);
        controlState.status = Idle;
        break;

    default:
        UsbHardware::epSetStalled(0, true);
    }
}
