#include "usb/usbcore.h"
#include "usb/usbhardware.h"
#include "usb/usbcontrol.h"
#include "usb/usbdevice.h"

#include "macros.h"
#include <string.h>

using namespace Usb;

const DeviceDescriptor *UsbCore::_dev;
const ConfigDescriptor *UsbCore::_conf;
const char **UsbCore::_strings;

uint16_t UsbCore::address;
uint16_t UsbCore::_config;

void UsbCore::init(const DeviceDescriptor *dev,
                const ConfigDescriptor *conf,
                const char **strings)
{
    _dev = dev;
    _conf = conf;
    _strings = strings;

    UsbHardware::init();
}

void UsbCore::reset()
{
    address = 0;
    _config = 0;
    UsbHardware::setAddress(0);

    UsbDevice::handleReset();
}

int UsbCore::getDescriptor(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->wValue >> 8) {

    case DescriptorDevice:
        *buf = (uint8_t*)UsbCore::devDescriptor();
        *len = MIN(*len, UsbCore::devDescriptor()->bLength);
        return 1;

    case DescriptorConfiguration: {
        uint8_t configIdx = req->wValue & 0xff;
        *buf = (uint8_t*)UsbCore::configDescriptor(configIdx);
        *len = MIN(UsbCore::configDescriptor(configIdx)->wTotalLength, *len);
        return 1;
    }

    case DescriptorString: {
        if (!UsbCore::stringSupport())
            return 0;

        StringDescriptor *sd = reinterpret_cast<StringDescriptor*>(*buf);
        uint8_t strIdx = req->wValue & 0xff;

        sd->bDescriptorType = DescriptorString;

        /*
         * Special handling for Windows - associate ourselves with the standard
         * WinUSB driver automatically.
         *
         * See http://sourceforge.net/apps/mediawiki/libwdi/index.php?title=WCID_devices
         */
        if (strIdx == 0xee) {
            sd->bLength = 18;
            sd->wstring[0] = 0x004d;    // M
            sd->wstring[1] = 0x0053;    // S
            sd->wstring[2] = 0x0046;    // F
            sd->wstring[3] = 0x0054;    // T
            sd->wstring[4] = 0x0031;    // 1
            sd->wstring[5] = 0x0030;    // 0
            sd->wstring[6] = 0x0030;    // 0

            sd->wstring[7] = 0x0055;    // Vendor Code & padding
            return 1;
        }

        // check for bogus string
        for (unsigned i = 0; i <= strIdx; i++) {
            if (UsbCore::string(i) == NULL)
                return 0;
        }

        sd->bLength = strlen(UsbCore::string(strIdx)) * 2 + 2;

        *buf = (uint8_t*)sd;
        *len = MIN(*len, sd->bLength);

        for (int i = 0; i < (*len / 2) - 1; i++) {
            sd->wstring[i] = UsbCore::string(strIdx)[i];
        }

        // string index 0 returns a list of languages
        if (strIdx == 0)
            sd->wstring[0] = 0x409; // US / English

        return 1;
    }

    }
    return 0;
}

int UsbCore::standardDeviceRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->bRequest) {
    case RequestSetAddress:
        // The actual address is only latched at the STATUS IN stage
        if ((req->bmRequestType != 0) || (req->wValue >= 128))
            return 0;

        address = req->wValue;
        UsbHardware::setAddress(address);
        return 1;

    case RequestSetConfiguration:
        if (req->wValue == _config)
            return 1;

        _config = req->wValue;
        UsbHardware::reset();
        UsbDevice::onConfigComplete(_config);
        return 1;

    case RequestGetConfiguration:
        *len = 1;
        (*buf)[0] = _config;
        return 1;

    case RequestGetDescriptor:
        return getDescriptor(req, buf, len);

    case RequestGetStatus:
        *len = 2;
        (*buf)[0] = 0;  // bit 0: self powered
        (*buf)[1] = 0;  // bit 1: remote wakeup
        return 1;
    }

    return 0;
}

/*
 * Standard requests destined for an interface.
 * We only support a single interface at the moment, assuming interface 0.
 */
int UsbCore::standardInterfaceRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    switch (req->bRequest) {
    case RequestGetInterface:
        *len = 1;
        (*buf)[0] = 0;          // TODO: support additional interfaces if needed
        return 1;

    case RequestSetInterface:
        if (req->wValue != 0)   // TODO: additional interfaces here as well
            return 0;
        *len = 0;
        return 1;

    case RequestGetStatus:
        *len = 2;
        (*buf)[0] = 0;
        (*buf)[1] = 0;
        return 1;
    }

    return 0;   // unhandled
}

/*
 * Standard requests targeted at a specific endpoint.
 * For now, we're mainly handling stall status.
 */
int UsbCore::standardEndpointRequest(SetupData *req, uint8_t **buf, uint16_t *len)
{
    uint8_t ep = req->wIndex;
    switch (req->bRequest) {
    case RequestClearFeature:
        if (req->wValue == FeatureEndpointHalt) {
            UsbHardware::epSetStalled(ep, false);
            return 1;
        }
        break;

    case RequestSetFeature:
        if (req->wValue == FeatureEndpointHalt) {
            UsbHardware::epSetStalled(ep, true);
            return 1;
        }
        break;

    case RequestGetStatus:
        *len = 2;
        (*buf)[0] = UsbHardware::epIsStalled(ep) ? 1 : 0;
        (*buf)[1] = 0;

        return 1;
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
