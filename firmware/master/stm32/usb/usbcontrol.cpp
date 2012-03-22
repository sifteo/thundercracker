#include "usbcontrol.h"
#include "usb/usbd.h"

using namespace Usb;

UsbControl::ControlState UsbControl::controlState;

int UsbControl::registerCallback(uint8_t type, uint8_t type_mask, controlCallback cb)
{
//    for (unsigned i = 0; i < MAX_USER_CONTROL_CALLBACK; i++) {
//        if (_usbd_device.user_control_callback[i].cb)
//            continue;

//        _usbd_device.user_control_callback[i].type = type;
//        _usbd_device.user_control_callback[i].type_mask = type_mask;
//        _usbd_device.user_control_callback[i].cb = cb;
//        return 0;
//    }

    return -1;
}

void UsbControl::sendChunk()
{
#if 0
    if (_usbd_device.desc->bMaxPacketSize0 < controlState.ctrl_len) {
        /* Data stage, normal transmission */
        usbd_ep_write_packet(0, controlState.ctrl_buf, _usbd_device.desc->bMaxPacketSize0);
        controlState.status = DataIn;
        controlState.ctrl_buf += _usbd_device.desc->bMaxPacketSize0;
        controlState.ctrl_len -= _usbd_device.desc->bMaxPacketSize0;
    }
    else {
        /* Data stage, end of transmission */
        usbd_ep_write_packet(0, controlState.buf, controlState.len);
        controlState.status = LastDataIn;
        controlState.len = 0;
        controlState.buf = NULL;
    }
#endif
}

int UsbControl::receiveChunk()
{
#if 0
    uint16_t packetsize = MIN(_usbd_device.desc->bMaxPacketSize0, controlState.req.wLength - controlState.len);
    uint16_t size = usbd_ep_read_packet(0, controlState.buf + controlState.len, packetsize);

    if (size != packetsize) {
        usbd_ep_stall_set(0, 1);
        return -1;
    }

    controlState.len += size;

    return packetsize;
#endif
    return 0;
}

int UsbControl::requestDispatch(SetupData *req)
{
#if 0
    struct user_control_callback *cb = _usbd_device.user_control_callback;

    /* Call user command hook function. */
    for (unsigned i = 0; i < MAX_USER_CONTROL_CALLBACK; i++) {
        if (cb[i].cb == NULL)
            break;

        if ((req->bmRequestType & cb[i].type_mask) == cb[i].type) {
            int result = cb[i].cb(req, &controlState.ctrl_buf,
                                    &controlState.ctrl_len,
                                    &controlState.complete);
            if (result)
                return result;
        }
    }

    /* Try standard request if not already handled. */
    return _usbd_standard_request(req, &controlState.ctrl_buf, &controlState.ctrl_len);
#endif
    return 0;
}

void UsbControl::setupRead(SetupData *req)
{
#if 0
    controlState.ctrl_buf = _usbd_device.ctrl_buf;
    controlState.ctrl_len = req->wLength;

    if (usb_control_request_dispatch(req)) {
        if (controlState.ctrl_len) {
            /* Go to data out stage if handled. */
            sendChunk();
        }
        else {
            /* Go to status stage if handled. */
            usbd_ep_write_packet(0, NULL, 0);
            controlState.status = StatusIn;
        }
    }
    else {
        usbd_ep_stall_set(0, 1);    // stall on failure
    }
#endif
}

void UsbControl::setupWrite(SetupData *req)
{
#if 0
    if (req->wLength > _usbd_device.ctrl_buf_len) {
        usbd_ep_stall_set(0, 1);
        return;
    }

    /* Buffer into which to write received data. */
    controlState.buf = _usbd_device.ctrl_buf;
    controlState.len = 0;
    /* Wait for DATA OUT stage. */
    if (req->wLength > _usbd_device.desc->bMaxPacketSize0)
        controlState.status = DataOut;
    else
        controlState.status = LastDataOut;
#endif
}

void UsbControl::setup(uint8_t ea)
{
#if 0
    struct usb_setup_data *req = &controlState.req;
    (void)ea;

    controlState.complete = NULL;

    if (usbd_ep_read_packet(0, req, 8) != 8) {
        usbd_ep_stall_set(0, 1);
        return;
    }

    if (req->wLength == 0)
        setupRead(req);
    else if (req->bmRequestType & 0x80)
        setupRead(req);
    else
        setupWrite(req);
#endif
}

void UsbControl::out(uint8_t ea)
{
#if 0
    (void)ea;

    switch (controlState.status) {
    case DataOut:
        if (recvChunk() < 0)
            break;
        if ((controlState.req.wLength - controlState.ctrl_len) <= _usbd_device.desc->bMaxPacketSize0)
            controlState.status = LastDataOut;
        break;
    case LastDataOut:
        if (recvChunk() < 0)
            break;
        /*
         * We have now received the full data payload.
         * Invoke callback to process.
         */
        if (requestDispatch(&controlState.req)) {
            /* Got to status stage on success. */
            usbd_ep_write_packet(0, NULL, 0);
            controlState.status = StatusIn;
        }
        else {
            usbd_ep_stall_set(0, 1);
        }
        break;
    case StatusOut:
        usbd_ep_read_packet(0, NULL, 0);
        controlState.status = Idle;
        if (controlState.complete) {
            controlState.complete(&controlState.req);
        }
        controlState.complete = NULL;
        break;
    default:
        usbd_ep_stall_set(0, 1);
    }
#endif
}

void UsbControl::in(uint8_t ea)
{
#if 0
    (void)ea;
    SetupData &req = controlState.req;

    switch (controlState.status) {
    case DATA_IN:
        sendChunk();
        break;
    case LAST_DATA_IN:
        controlState.status = STATUS_OUT;
        break;
    case STATUS_IN:
        if (controlState.complete)
            controlState.complete(&controlState.req);

        /* Exception: Handle SET ADDRESS function here... */
        if ((req.bmRequestType == 0) &&
            (req.bRequest == USB_REQ_SET_ADDRESS))
            _usbd_hw_set_address(req.wValue);
        controlState.status = Idle;
        break;
    default:
        usbd_ep_stall_set(0, 1);
    }
#endif
}
