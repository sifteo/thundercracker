/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usb.h"
#include "usb/usbd.h"
#include "usb/usbhardware.h"
#include "usb/usbdefs.h"

#include "hardware.h"
#include "usart.h"
#include "tasks.h"
#include "assetmanager.h"

static const Usb::DeviceDescriptor dev = {
    sizeof(Usb::DeviceDescriptor),  // bLength
    Usb::DescriptorDevice,          // bDescriptorType
    0x0200,                         // bcdUSB
    0xFF,                           // bDeviceClass
    0,                              // bDeviceSubClass
    0,                              // bDeviceProtocol
    64,                             // bMaxPacketSize0
    0xCAFE,                         // idVendor
    0xCAFE,                         // idProduct
    0x0200,                         // bcdDevice
    1,                              // iManufacturer
    2,                              // iProduct
    3,                              // iSerialNumber
    1                               // bNumConfigurations
};

static const Usb::InterfaceDescriptor iface = {
    sizeof(Usb::InterfaceDescriptor),   // bLength
    Usb::DescriptorInterface,           // bDescriptorType
    0,                                  // bInterfaceNumber
    0,                                  // bAlternateSetting
    0,                                  // bNumEndpoints
    0xFF,                               // bInterfaceClass
    0,                                  // bInterfaceSubClass
    0,                                  // bInterfaceProtocol
    0                                   // iInterface
};

static const Usb::ConfigDescriptor config = {
    sizeof(Usb::ConfigDescriptor),  // bLength
    Usb::DescriptorConfiguration,   // bDescriptorType
    0,                              // wTotalLength
    1,                              // bNumInterfaces
    1,                              // bConfigurationValue
    0,                              // iConfiguration
    0x80,                           // bmAttributes
    0x32                            // bMaxPower
};

static const char *descriptorStrings[] = {
    "x",
    "Sifteo Inc.",
    "Thundercracker",
    "1001",
};

/*
    Called from within Tasks::work() to process usb OUT data on the
    main thread.
*/
void UsbDevice::handleOUTData(void *p) {

}

/*
    Called from within Tasks::work() to handle a usb IN event on the main thread.
    TBD whether this is actually helpful.
*/
void UsbDevice::handleINData(void *p) {
    (void)p;
}

void UsbDevice::init() {
    Usbd::init(&dev, &config, descriptorStrings);
}

int UsbDevice::write(const uint8_t *buf, unsigned len)
{
    return 0;
}

int UsbDevice::read(uint8_t *buf, unsigned len)
{
    return 0;
}

IRQ_HANDLER ISR_UsbOtg_FS() {
    UsbHardware::isr();
}
