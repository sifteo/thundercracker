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

    static int getDescriptor(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    // devices write their own string descriptors, but make it easy for the common case
    static int writeAsciiDescriptor(uint16_t *dst, const char *src, unsigned srclen);

    static int standardDeviceRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardInterfaceRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardEndpointRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

private:
    static const Usb::DeviceDescriptor *_dev;
    static const Usb::ConfigDescriptor *_conf;

    static uint16_t address;
    static uint16_t _config;
};

#endif // USBCORE_H
