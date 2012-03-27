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
    0xff,                           // bDeviceClass - 0xff == vendor specific
    0,                              // bDeviceSubClass
    0,                              // bDeviceProtocol
    64,                             // bMaxPacketSize0
    UsbDevice::VendorID,            // idVendor
    UsbDevice::ProductID,           // idProduct
    0x0200,                         // bcdDevice
    1,                              // iManufacturer
    2,                              // iProduct
    0,                              // iSerialNumber
    1                               // bNumConfigurations
};

/*
 * Configuration descriptor - the Usb::ConfigDescriptor must be first in the
 * struct, such that we can cast it correctly.
 */
static const struct {

    Usb::ConfigDescriptor       configDescriptor;
    Usb::InterfaceDescriptor    interfaceDescriptor;
    Usb::EndpointDescriptor     inEpDescriptor;
    Usb::EndpointDescriptor     outEpDescriptor;

} __attribute__((packed)) configurationBlock = {

    // configDescriptor
    {
        sizeof(Usb::ConfigDescriptor),      // bLength
        Usb::DescriptorConfiguration,       // bDescriptorType
        sizeof(configurationBlock),         // wTotalLength
        1,                                  // bNumInterfaces
        1,                                  // bConfigurationValue
        0,                                  // iConfiguration
        0x80,                               // bmAttributes
        0x32                                // bMaxPower
    },
    // interfaceDescriptor
    {
        sizeof(Usb::InterfaceDescriptor),   // bLength
        Usb::DescriptorInterface,           // bDescriptorType
        0,                                  // bInterfaceNumber
        0,                                  // bAlternateSetting
        2,                                  // bNumEndpoints
        0xFF,                               // bInterfaceClass
        0,                                  // bInterfaceSubClass
        0,                                  // bInterfaceProtocol
        0                                   // iInterface
    },
    // inEpDescriptor
    {
        sizeof(Usb::EndpointDescriptor),    // bLength
        Usb::DescriptorEndpoint,            // bDescriptorType
        UsbDevice::InEpAddr,                // bEndpointAddress
        Usb::EpAttrBulk,                    // bmAttributes
        64,                                 // wMaxPacketSize
        1,                                  // bInterval
    },
    // outEpDescriptor
    {
        sizeof(Usb::EndpointDescriptor),    // bLength
        Usb::DescriptorEndpoint,            // bDescriptorType
        UsbDevice::OutEpAddr,               // bEndpointAddress
        Usb::EpAttrBulk,                    // bmAttributes
        64,                                 // wMaxPacketSize
        1,                                  // bInterval
    }
};

static const char *descriptorStrings[] = {
    "x",
    "Sifteo Inc.",
    "Thundercracker",
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
    Usbd::init(&dev, (Usb::ConfigDescriptor*)&configurationBlock, descriptorStrings);
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
