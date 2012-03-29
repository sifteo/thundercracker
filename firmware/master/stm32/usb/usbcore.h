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

    static inline const Usb::DeviceDescriptor* devDescriptor() {
        return _dev;
    }

    static inline const Usb::ConfigDescriptor* configDescriptor(uint8_t idx) {
        // TODO: calculate offset of subsequent configurations
        return _conf;
    }

    static inline bool stringSupport() {
        return _strings != 0;
    }

    static inline const char* string(uint8_t idx) {
        return _strings[idx];
    }

    static void setAddress(uint16_t addr);
    static void setConfig(uint16_t cfg);

    static int getDescriptor(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static int setAddress(Usb::SetupData *req);

    static int setConfiguration(Usb::SetupData *req);
    static int getConfiguration(uint8_t **buf, uint16_t *len);

    static int setInterface(Usb::SetupData *req, uint16_t *len);
    static int getInterface(uint8_t **buf, uint16_t *len);

    static int standardDeviceRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardInterfaceRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardEndpointRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);
    static int standardRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

private:
    static int getDeviceStatus(uint8_t **buf, uint16_t *len);
    static int getInterfaceStatus(uint8_t **buf, uint16_t *len);
    static int getEndpointStatus(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static const Usb::DeviceDescriptor *_dev;
    static const Usb::ConfigDescriptor *_conf;
    static const char **_strings;

    static uint16_t address;
    static uint16_t _config;
};

#endif // USBCORE_H
