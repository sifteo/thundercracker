#ifndef USBCORE_H
#define USBCORE_H

#include "usb/usbdefs.h"

class UsbCore
{
public:
    static void init(const Usb::DeviceDescriptor *dev,
                        const Usb::ConfigDescriptor *conf,
                        const char **strings);
    static void reset();

    static ALWAYS_INLINE const Usb::DeviceDescriptor* devDescriptor() {
        return _dev;
    }

    static ALWAYS_INLINE const Usb::ConfigDescriptor* configDescriptor(uint8_t idx) {
        // TODO: calculate offset of subsequent configurations
        return _conf;
    }

    static ALWAYS_INLINE bool stringSupport() {
        return _strings != 0;
    }

    static ALWAYS_INLINE const char* string(uint8_t idx) {
        return _strings[idx];
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
    static const char **_strings;

    static uint16_t address;
    static uint16_t _config;
};

#endif // USBCORE_H
