#include "usbcore.h"

using namespace Usb;

UsbCore::UsbCore()
{
}

uint16_t UsbCore::buildConfigDescriptor(uint8_t index, uint8_t *buf, uint16_t len)
{
#if 0
    uint8_t *tmpbuf = buf;
    const ConfigDescriptor *cfg = &_usbd_device.config[index];
    uint16_t count, total = 0, totallen = 0;

    memcpy(buf, cfg, count = MIN(len, cfg->bLength));
    buf += count;
    len -= count;
    total += count;
    totallen += cfg->bLength;

    // iterate interfaces
    for (unsigned i = 0; i < cfg->bNumInterfaces; i++) {

        // iterate alternate settings
        for (unsigned j = 0; j < cfg->interface[i].num_altsetting; j++) {

            const InterfaceDescriptor *iface = &cfg->interface[i].altsetting[j];

            // Copy interface descriptor
            memcpy(buf, iface, count = MIN(len, iface->bLength));
            buf += count;
            len -= count;
            total += count;
            totallen += iface->bLength;

            // Copy extra bytes (function descriptors)
            memcpy(buf, iface->extra, count = MIN(len, iface->extralen));
            buf += count;
            len -= count;
            total += count;
            totallen += iface->extralen;

            // iterate endpoints
            for (unsigned k = 0; k < iface->bNumEndpoints; k++) {
                const EndpointDescriptor *ep = &iface->endpoint[k];
                memcpy(buf, ep, count = MIN(len, ep->bLength));
                buf += count;
                len -= count;
                total += count;
                totallen += ep->bLength;
            }
        }
    }

    // Fill in wTotalLength
    *reinterpret_cast<uint16_t*>(tmpbuf + 2) = totallen;

    return total;
#endif
    return 0;
}

int UsbCore::getDescriptor(SetupData *req, uint8_t **buf, uint16_t *len)
{
#if 0
    switch (req->wValue >> 8) {
    case USB_DT_DEVICE:
        *buf = (uint8_t *) _usbd_device.desc;
        *len = MIN(*len, _usbd_device.desc->bLength);
        return 1;
    case USB_DT_CONFIGURATION:
        *buf = _usbd_device.ctrl_buf;
        *len = build_config_descriptor(req->wValue & 0xff, *buf, *len);
        return 1;
    case USB_DT_STRING: {
        struct usb_string_descriptor *sd;
        sd = reinterpret_cast<StringDescriptor*>(_usbd_device.ctrl_buf);

        if (!_usbd_device.strings) {
            return 0; // no string support
        }

        for (unsigned i = 0; i <= (req->wValue & 0xff); i++) {
            if (_usbd_device.strings[i] == NULL)
                return 0;
        }

        sd->bLength = strlen(_usbd_device.strings[req->wValue & 0xff]) * 2 + 2;
        sd->bDescriptorType = USB_DT_STRING;

        *buf = (uint8_t*)sd;
        *len = MIN(*len, sd->bLength);

        for (i = 0; i < (*len / 2) - 1; i++) {
            sd->wData[i] = _usbd_device.strings[req->wValue & 0xff][i];
        }

        /* Send sane Language ID descriptor... */
        if ((req->wValue & 0xff) == 0)
            sd->wData[0] = 0x409;

        return 1;
    }
    }
#endif
    return 0;
}

int UsbCore::setAddress(SetupData *req, uint8_t **buf, uint16_t *len)
{
#if 0
    (void)req;
    (void)buf;
    (void)len;

    // The actual address is only latched at the STATUS IN stage
    if ((req->bmRequestType != 0) || (req->wValue >= 128))
        return 0;

    _usbd_device.current_address = req->wValue;

    /*
     * Special workaround for STM32F10[57] that require the address
     * to be set here. This is undocumented!
     */
    if (_usbd_device.driver == &stm32f107_usb_driver)
        _usbd_device.driver->set_address(req->wValue);

#endif
    return 1;
}

int UsbCore::setConfiguration(SetupData *req, uint8_t **buf, uint16_t *len)
{
#if 0
    (void)req;
    (void)buf;
    (void)len;

    /* Is this correct, or should we reset alternate settings. */
    if (req->wValue == _usbd_device.current_config)
        return 1;

    _usbd_device.current_config = req->wValue;

    /* Reset all endpoints. */
    _usbd_hw_endpoints_reset();

    if (_usbd_device.user_callback_set_config) {
        /*
         * Flush control callbacks. These will be reregistered
         * by the user handler.
         */
        for (unsigned i = 0; i < MAX_USER_CONTROL_CALLBACK; i++)
            _usbd_device.user_control_callback[i].cb = NULL;

        _usbd_device.user_callback_set_config(req->wValue);
    }

#endif
    return 1;
}

int UsbCore::getConfiguration(SetupData *req, uint8_t **buf, uint16_t *len)
{
    (void)req;

    if (*len > 1)
        *len = 1;
//    (*buf)[0] = _usbd_device.current_config;

    return 1;
}

int UsbCore::setInterface(SetupData *req, uint8_t **buf, uint16_t *len)
{
    (void)req;
    (void)buf;

    /* FIXME: Adapt if we have more than one interface. */
    if (req->wValue != 0)
        return 0;
    *len = 0;

    return 1;
}

int UsbCore::getInterface(SetupData *req, uint8_t **buf, uint16_t *len)
{
    (void)req;
    (void)buf;

    /* FIXME: Adapt if we have more than one interface. */
    *len = 1;
    (*buf)[0] = 0;

    return 1;
}

