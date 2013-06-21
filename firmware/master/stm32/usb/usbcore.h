#ifndef USBCORE_H
#define USBCORE_H

#include "usb/usbdefs.h"

class UsbCore
{
public:

    struct Config {
        bool enableSOF;
    };

    static void init(const Usb::DeviceDescriptor *dev,
                     const Usb::ConfigDescriptor *conf,
                     const Config & cfg);
    static void reset();

    static ALWAYS_INLINE const Usb::DeviceDescriptor* devDescriptor() {
        return _dev;
    }

    static ALWAYS_INLINE const Usb::ConfigDescriptor* configDescriptor(uint8_t idx) {
        // TODO: calculate offset of subsequent configurations
        return _conf;
    }

private:
    static const Usb::DeviceDescriptor *_dev;
    static const Usb::ConfigDescriptor *_conf;

    static uint16_t address;
    static uint16_t _config;
};

#endif // USBCORE_H
