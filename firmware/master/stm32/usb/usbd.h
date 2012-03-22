#ifndef USBD_H
#define USBD_H

#include "usb/usbdefs.h"

class Usbd {
public:

    static void init(const Usb::DeviceDescriptor *dev,
                        const Usb::ConfigDescriptor *conf,
                        const char **strings);
    static void reset();

    static inline const Usb::DeviceDescriptor* devDescriptor() {
        return _dev;
    }

    static inline const Usb::ConfigDescriptor & configDescriptor(uint8_t idx) {
        return _conf[idx];
    }

    static inline bool stringSupport() {
        return _strings != 0;
    }

    static inline const char* string(uint8_t idx) {
        return _strings[idx];
    }

    static void setAddress(uint16_t addr);

    static uint8_t ctrlBuf[128];

private:
    static const unsigned MAX_USER_CONTROL_CALLBACK = 4;

    struct UserControlCallback_t {
//        struct UserControlCallback_t cb;
        uint8_t type;
        uint8_t type_mask;
    } userControlCallback[MAX_USER_CONTROL_CALLBACK];

    static const Usb::DeviceDescriptor *_dev;
    static const Usb::ConfigDescriptor *_conf;
    static const char **_strings;

    static uint16_t address;
};

#endif // USBD_H
