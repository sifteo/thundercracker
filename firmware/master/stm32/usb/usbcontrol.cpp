#include "usbcontrol.h"
#include "usb/usbhardware.h"
#include "usb/usbcore.h"
#include "usb.h"

#include "macros.h"

using namespace Usb;

UsbControl::ControlState UsbControl::controlState;

void UsbControl::sendChunk()
{
    const DeviceDescriptor *dd = UsbCore::devDescriptor();
    if (dd->bMaxPacketSize0 < controlState.len) {
        // Data stage, normal transmission
        UsbHardware::epWritePacket(0, controlState.pdata, dd->bMaxPacketSize0);
        controlState.status = DataIn;
        controlState.pdata += dd->bMaxPacketSize0;
        controlState.len -= dd->bMaxPacketSize0;
    }
    else {
        // Data stage, end of transmission
        UsbHardware::epWritePacket(0, controlState.pdata, controlState.len);
        controlState.status = LastDataIn;
        controlState.len = 0;
        controlState.pdata = 0;
    }
}

int UsbControl::receiveChunk()
{
    uint16_t packetsize = MIN(UsbCore::devDescriptor()->bMaxPacketSize0, controlState.req.wLength - controlState.len);
    uint16_t size = UsbHardware::epReadPacket(0, controlState.pdata + controlState.len, packetsize);

    if (size != packetsize) {
        UsbHardware::epSetStalled(0, true);
        return -1;
    }

    controlState.len += size;

    return packetsize;
}

int UsbControl::requestDispatch(SetupData *req)
{

    int result = UsbDevice::controlRequest(req, &controlState.pdata, &controlState.len);
    if (result)
        return result;

    return UsbCore::standardRequest(req, &controlState.pdata, &controlState.len);
}

void UsbControl::setupRead(SetupData *req)
{
    controlState.pdata = controlState.buf;
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
    if (req->wLength > controlState.len) {
        UsbHardware::epSetStalled(0, true);
        return;
    }

    // Buffer into which to write received data
    controlState.pdata = controlState.buf;
    controlState.len = 0;

    // Wait for DATA OUT stage
    if (req->wLength > UsbCore::devDescriptor()->bMaxPacketSize0)
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
        setup();
        break;
    default:
        return false;
    }
    return true;
}

void UsbControl::setup()
{
    SetupData *req = &controlState.req;

    if (UsbHardware::epReadPacket(0, req, 8) != 8) {
        UsbHardware::epSetStalled(0, true);
        return;
    }

    if (req->wLength == 0 || (req->bmRequestType & 0x80))
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
        if ((controlState.req.wLength - controlState.len) <= UsbCore::devDescriptor()->bMaxPacketSize0)
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
        // Exception: Handle SET ADDRESS function here...
        if ((req.bmRequestType == 0) && (req.bRequest == RequestSetAddress))
            UsbHardware::setAddress(req.wValue);
        controlState.status = Idle;
        break;

    default:
        UsbHardware::epSetStalled(0, true);
    }
}
