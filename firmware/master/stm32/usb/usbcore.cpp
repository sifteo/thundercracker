#include "usbcore.h"
#include "macros.h"
#include <string.h>

#include "usb/usbhardware.h"
#include "usb/usbdriver.h"

using namespace Usb;

UsbCore::UsbCore()
{
}

uint16_t UsbCore::buildConfigDescriptor(uint8_t index, uint8_t *buf, uint16_t len)
{
    uint8_t *tmpbuf = buf;
    const ConfigDescriptor &cfg = Usbd::configDescriptor(index);
    uint16_t count, total = 0, totallen = 0;

    memcpy(buf, &cfg, count = MIN(len, cfg.bLength));
    buf += count;
    len -= count;
    total += count;
    totallen += cfg.bLength;

    // iterate interfaces
    for (unsigned i = 0; i < cfg.bNumInterfaces; i++) {
#if 0
        // iterate alternate settings
        for (unsigned j = 0; j < cfg.interface[i].num_altsetting; j++) {

            const InterfaceDescriptor *iface = &cfg.interface[i].altsetting[j];

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
#endif
    }

    // Fill in wTotalLength
    *reinterpret_cast<uint16_t*>(tmpbuf + 2) = totallen;

    return total;
}

int UsbCore::getDescriptor(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->wValue >> 8) {

    case DescriptorDevice: {
        const DeviceDescriptor *d = Usbd::devDescriptor();
//        *buf = reinterpret_cast<const uint8_t*>(d);
        *len = MIN(*len, d->bLength);
        return 1;
    }

    case DescriptorConfiguration:
        *buf = Usbd::ctrlBuf;
        *len = buildConfigDescriptor(req->wValue & 0xff, *buf, *len);
        return 1;

    case DescriptorString: {
        if (!Usbd::stringSupport())
            return 0;

        StringDescriptor *sd = reinterpret_cast<StringDescriptor*>(Usbd::ctrlBuf);

        // check for bogus string
        for (unsigned i = 0; i <= (req->wValue & 0xff); i++) {
            if (Usbd::string(i) == NULL)
                return 0;
        }

        sd->bLength = strlen(Usbd::string(req->wValue & 0xff)) * 2 + 2;
        sd->bDescriptorType = DescriptorString;

        *buf = (uint8_t*)sd;
        *len = MIN(*len, sd->bLength);

        // todo: convert to mem or strcpy?
        for (int i = 0; i < (*len / 2) - 1; i++) {
            sd->wstring[i] = Usbd::string(req->wValue & 0xff)[i];
        }

        // Send sane Language ID descriptor...
        if ((req->wValue & 0xff) == 0)
            sd->wstring[0] = 0x409;

        return 1;
    }

    }
    return 0;
}

int UsbCore::setAddress(SetupData *req)
{
    // The actual address is only latched at the STATUS IN stage
    if ((req->bmRequestType != 0) || (req->wValue >= 128))
        return 0;

    Usbd::setAddress(req->wValue);

    return 1;
}

int UsbCore::setConfiguration(SetupData *req)
{
    /* Is this correct, or should we reset alternate settings. */
    if (req->wValue == Usbd::config())
        return 1;

    Usbd::setConfig(req->wValue);
    UsbHardware::epReset();
    UsbDriver::setConfig(req->wValue);

    return 1;
}

int UsbCore::getConfiguration(SetupData *req, uint8_t **buf, uint16_t *len)
{
    (void)req;

    if (*len > 1)
        *len = 1;
    (*buf)[0] = Usbd::config();

    return 1;
}

int UsbCore::setInterface(SetupData *req, uint16_t *len)
{
    /* FIXME: Adapt if we have more than one interface. */
    if (req->wValue != 0)
        return 0;
    *len = 0;

    return 1;
}

int UsbCore::getInterface(uint8_t **buf, uint16_t *len)
{
    /* FIXME: Adapt if we have more than one interface. */
    *len = 1;
    (*buf)[0] = 0;

    return 1;
}

int UsbCore::getDeviceStatus(uint8_t **buf, uint16_t *len)
{
    // bit 0: self powered
    // bit 1: remote wakeup
    if (*len > 2)
        *len = 2;
    (*buf)[0] = 0;
    (*buf)[1] = 0;

    return 1;
}

int UsbCore::getInterfaceStatus(uint8_t **buf, uint16_t *len)
{
    // not defined
    if (*len > 2)
        *len = 2;
    (*buf)[0] = 0;
    (*buf)[1] = 0;

    return 1;
}

int UsbCore::getEndpointStatus(SetupData *req, uint8_t **buf, uint16_t *len)
{
    if (*len > 2)
        *len = 2;
    (*buf)[0] = UsbHardware::epIsStalled(req->wIndex) ? 1 : 0;
    (*buf)[1] = 0;

    return 1;
}

int UsbCore::standardDeviceRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->bRequest) {
    case RequestSetAddress:
        /*
         * SET ADDRESS is an exception.
         * It is only processed at STATUS stage.
         */
        return setAddress(req);

    case RequestSetConfiguration:
        return setConfiguration(req);

    case RequestGetConfiguration:
        return getConfiguration(req, buf, len);

    case RequestGetDescriptor:
        return getDescriptor(req, buf, len);

    case RequestGetStatus:
        /*
         * GET_STATUS always responds with zero reply.
         * The application may override this behaviour.
         */
        return getDeviceStatus(buf, len);
    }

    return 0;
}

int UsbCore::standardInterfaceRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->bRequest) {
    case RequestGetInterface:
        return getInterface(buf, len);

    case RequestSetInterface:
        return setInterface(req, len);

    case RequestGetStatus:
        return getInterfaceStatus(buf, len);
    }

    return 0;
}

int UsbCore::standardEndpointRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->bRequest) {
    case RequestClearFeature:
        if (req->wValue == FeatureEndpointHalt) {
            UsbHardware::epSetStalled(req->wIndex, false);
            return 1;
        }
        break;

    case RequestSetFeature:
        if (req->wValue == FeatureEndpointHalt) {
            UsbHardware::epSetStalled(req->wIndex, true);
            return 1;
        }
        break;

    case RequestGetStatus:
        return getEndpointStatus(req, buf, len);
    }

    return 0;
}

int UsbCore::standardRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    const unsigned StandardRequestTest = 0;
    /* FIXME: Have class/vendor requests as well. */
    if ((req->bmRequestType & ReqTypeType) != StandardRequestTest)
        return 0;

    const unsigned RequestTypeTest = 0x1F;
    switch (req->bmRequestType & RequestTypeTest) {
    case ReqTypeDevice:
        return standardDeviceRequest(req, buf, len);

    case ReqTypeInterface:
        return standardInterfaceRequest(req, buf, len);

    case ReqTypeEndpoint:
        return standardEndpointRequest(req, buf, len);
    }

    return 0;
}
