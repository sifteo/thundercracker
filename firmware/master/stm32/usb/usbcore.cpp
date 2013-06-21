#include "usb/usbcore.h"
#include "usb/usbhardware.h"
#include "usb/usbcontrol.h"
#include "usb/usbdevice.h"

#include "macros.h"
#include <string.h>

using namespace Usb;

const DeviceDescriptor *UsbCore::_dev;
const ConfigDescriptor *UsbCore::_conf;

uint16_t UsbCore::address;
uint16_t UsbCore::_config;

void UsbCore::init(const DeviceDescriptor *dev,
                   const ConfigDescriptor *conf,
                   const Config & cfg)
{
    _dev = dev;
    _conf = conf;

    UsbHardware::init(cfg);
}

void UsbCore::reset()
{
    address = 0;
    _config = 0;
    UsbHardware::setAddress(0);

    UsbDevice::handleReset();
}
