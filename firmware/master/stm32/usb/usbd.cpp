
#include "usb/usbd.h"
#include "usb/usbhardware.h"
#include "usb/usbdriver.h"

using namespace Usb;

const DeviceDescriptor *Usbd::_dev;
const ConfigDescriptor *Usbd::_conf;
const char **Usbd::_strings;

uint16_t Usbd::address;
uint16_t Usbd::_config;

void Usbd::init(const DeviceDescriptor *dev,
                const ConfigDescriptor *conf,
                const char **strings)
{
    _dev = dev;
    _conf = conf;
    _strings = strings;

    UsbHardware::init();
}

void Usbd::reset()
{
    address = 0;
    _config = 0;
    UsbHardware::epSetup(0, EpAttrControl, 64);
    UsbHardware::setAddress(0);

    UsbDriver::handleReset();
}

void Usbd::setAddress(uint16_t addr)
{
    address = addr;
    UsbHardware::setAddress(addr);
}

void Usbd::setConfig(uint16_t cfg)
{
    _config = cfg;
}
