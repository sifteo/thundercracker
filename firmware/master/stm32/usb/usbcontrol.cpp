#include "usbcontrol.h"
#include "usb/usbhardware.h"
#include "usb/usbcore.h"
#include "usb/usbdevice.h"

#include "macros.h"

using namespace Usb;

UsbControl::ControlState UsbControl::controlState;

static const uint8_t zeroStatus[] = {0x00, 0x00};
static const uint8_t activeStatus[] ={0x00, 0x00};
static const uint8_t haltedStatus[] = {0x01, 0x00};

bool UsbControl::setupHandler()
{
    /*
     * Our job here is to prepare our setup object based
     * on the details of the received setup packet.
     *
     * Subsequent steps will actually perform the required I/O.
     */

    uint8_t ep;
    const uint8_t *descriptor;
    unsigned descLen;

    uint16_t request = (controlState.req.bRequest << 8) |
                       (controlState.req.bmRequestType & (ReqTypeMask | ReqTypeRecipientMask));

    switch (request) {

    case (RequestGetStatus << 8) | ReqTypeDevice:
        controlState.setupTransfer(&controlState.status, sizeof controlState.status);
        return true;

    case (RequestClearFeature << 8) | ReqTypeDevice:
        if ((controlState.req.wValue & 0xff) == FeatureDeviceRemoteWakeup) {
            controlState.status &= ~(1 << FeatureDeviceRemoteWakeup);
            controlState.setupTransfer(NULL, 0);
            return true;
        }
        // other features not handled
        return false;

    case (RequestSetFeature << 8) | ReqTypeDevice:
        if ((controlState.req.wValue & 0xff) == FeatureDeviceRemoteWakeup) {
            controlState.status |= (1 << FeatureDeviceRemoteWakeup);
            controlState.setupTransfer(NULL, 0);
            return true;
        }
        // other features not handled
        return false;

    case (RequestSetAddress << 8) | ReqTypeDevice:
        UsbHardware::setAddress(controlState.req.wValue);
        controlState.setupTransfer(NULL, 0);
        return true;

    case (RequestGetDescriptor << 8) | ReqTypeDevice:

        descLen = 0;

        switch (controlState.req.wValue >> 8) {
        case DescriptorString:
            descLen = UsbDevice::getStringDescriptor(controlState.req.wValue & 0xff, &descriptor);
            break;

        case DescriptorDevice:
            descriptor = (uint8_t*)UsbCore::devDescriptor();
            descLen = UsbCore::devDescriptor()->bLength;
            break;

        case DescriptorConfiguration:
            descriptor = (uint8_t*)UsbCore::configDescriptor(controlState.req.wValue & 0xff);
            descLen = UsbCore::configDescriptor(controlState.req.wValue & 0xff)->wTotalLength;
            break;
        }

        if (descLen) {
            controlState.setupTransfer(descriptor, descLen);
            return true;
        }
        return false;

    case (UsbDevice::WINUSB_COMPATIBLE_ID << 8) | ReqTypeVendor:
        if (controlState.req.wIndex == 0x04) {
            descLen = UsbDevice::getCompatIDDescriptor(&descriptor);
            controlState.setupTransfer(descriptor, descLen);
            return true;
        }
        return false;

    case (RequestGetConfiguration << 8) | ReqTypeDevice:
        controlState.setupTransfer(&controlState.configuration, 1);
        return true;

    case (RequestSetConfiguration << 8) | ReqTypeDevice:
        if (controlState.configuration != controlState.req.wValue) {
            controlState.configuration = controlState.req.wValue;
            if (controlState.configuration == 0)
                controlState.state = Selected;
            else
                controlState.state = Active;

            UsbHardware::reset();
            UsbDevice::onConfigComplete(controlState.configuration);
        }

        controlState.setupTransfer(NULL, 0);
        return true;

    case (RequestGetStatus << 8) | ReqTypeInterface:
    case (RequestSyncFrame << 8) | ReqTypeEndpoint:
        controlState.setupTransfer(zeroStatus, sizeof zeroStatus);
        return true;

    case (RequestGetStatus << 8) | ReqTypeEndpoint:
        ep = controlState.req.wIndex;
        if (UsbHardware::epIsStalled(ep)) {
            controlState.setupTransfer(haltedStatus, sizeof haltedStatus);
        } else {
            controlState.setupTransfer(activeStatus, sizeof activeStatus);
        }
        // XXX: handle disabled endpoints
        return true;

    case (RequestClearFeature << 8) | ReqTypeEndpoint:
        if (controlState.req.wValue != FeatureEndpointHalt)
            return false;

        // Clear the EP status, only valid for non-EP0 endpoints
        ep = controlState.req.wIndex;
        if (ep & 0x0F) {
            UsbHardware::epClearStall(ep);
        }
        controlState.setupTransfer(NULL, 0);
        return true;

    case (RequestSetFeature << 8) | ReqTypeEndpoint:
        if (controlState.req.wValue != FeatureEndpointHalt)
            return false;

        // Stall the EP, only valid for non-EP0 endpoints
        ep = controlState.req.wIndex;
        if (ep & 0x0F) {
            UsbHardware::epStall(ep);
        }
        controlState.setupTransfer(NULL, 0);
        return true;
    }

    return false;
}

void UsbControl::setup()
{
    /*
     * Entry point for all setup packets.
     */

    SetupData *req = &controlState.req;
    if (UsbHardware::epReadPacket(0, req, sizeof(*req)) != sizeof(*req)) {
        setErrorState();
        return;
    }

    if (!setupHandler()) {
        setErrorState();
        return;
    }

    // our data pointer and size have been set up, now kick off the transfer.
    if (reqIsIN(req->bmRequestType)) {
        if (controlState.len) {
            unsigned chunk = MIN(controlState.req.wLength, controlState.len);
            chunk = MIN(chunk, UsbCore::devDescriptor()->bMaxPacketSize0);

            UsbHardware::epWritePacket(0, controlState.pdata, chunk);
            controlState.ep0Status = EP0_TX;
//            controlState.pdata += chunk;
//            controlState.len -= chunk;
        } else {
            // no data to send, wait for zero-length status packet
            UART("wait for zlp\r\n");
            controlState.ep0Status = EP0_WAITING_STS;
        }

    } else {
        // OUT packet handling
        if (controlState.len) {
            UART("setup OUT: "); UART_HEX(controlState.len); UART("\r\n");
            controlState.ep0Status = EP0_RX;
        } else {
            UsbHardware::epWritePacket(0, NULL, 0);
            controlState.ep0Status = EP0_SENDING_STS;
        }
    }
}

void UsbControl::out()
{
    /*
     * An OUT packet for ep0 has arrived.
     * Update our state accordingly.
     */

    switch (controlState.ep0Status) {
    case EP0_RX:
        // Receive phase over, sending the zero sized status packet.
        controlState.ep0Status = EP0_SENDING_STS;
        UsbHardware::epWritePacket(0, NULL, 0);
        return;

    case EP0_WAITING_STS:
        controlState.ep0Status = EP0_WAITING_SETUP;
        return;

    default:
        setErrorState();
        break;
    }
}

void UsbControl::in()
{
    /*
     * An IN packet transmission has just completed.
     * Update our state accordingly.
     */

    unsigned max;

    switch (controlState.ep0Status) {
    case EP0_TX:
        max = controlState.req.wLength;
        /* If the transmitted size is less than the requested size and it is a
         multiple of the maximum packet size then a zero size packet must be
         transmitted.*/

        if ((controlState.len < max) && ((controlState.len % 64) == 0)) {
            UsbHardware::epWritePacket(0, NULL, 0);
            controlState.ep0Status = EP0_WAITING_TX0;
            return;
        }
        // Falls in

    case EP0_WAITING_TX0:
        // Transmit phase over, receiving the zero sized status packet.
        controlState.ep0Status = EP0_WAITING_STS;
        return;

    case EP0_SENDING_STS:
        // Status packet sent, invoking the callback if defined.
        controlState.ep0Status = EP0_WAITING_SETUP;
        return;

    default:
        setErrorState();
        break;
    }
}

void UsbControl::setErrorState()
{
    UsbHardware::epStall(0);
    UsbHardware::epStall(0 | 0x80);
    controlState.ep0Status = EP0_ERROR;
}
