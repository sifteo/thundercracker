
#include "usb/usbd.h"
#include "usb/usbhardware.h"
#include "usb/usbdriver.h"

using namespace Usb;

const DeviceDescriptor *Usbd::_dev;
const ConfigDescriptor *Usbd::_conf;
const char **Usbd::_strings;

uint8_t Usbd::ctrlBuf[128];
uint16_t Usbd::address;
uint16_t Usbd::_config;

void Usbd::init(const DeviceDescriptor *dev,
                const ConfigDescriptor *conf,
                const char **strings)
{
    _dev = dev;
    _conf = conf;
    _strings = strings;
//    _usbd_device.ctrl_buf = usbd_control_buffer;
//    _usbd_device.ctrl_buf_len = sizeof(usbd_control_buffer);

    UsbHardware::init();

//    _usbd_device.user_callback_ctr[0][USB_TRANSACTION_SETUP] = _usbd_control_setup;
//    _usbd_device.user_callback_ctr[0][USB_TRANSACTION_OUT] = _usbd_control_out;
//    _usbd_device.user_callback_ctr[0][USB_TRANSACTION_IN] = _usbd_control_in;
}

void Usbd::reset()
{
//    _usbd_device.current_address = 0;
//	_usbd_device.current_config = 0;
//	usbd_ep_setup(0, USB_ENDPOINT_ATTR_CONTROL, 64, NULL);
//	_usbd_hw_set_address(0);
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
