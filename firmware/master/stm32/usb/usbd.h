#ifndef USBD_H
#define USBD_H

#include "usb/usbdefs.h"

class Usbd {
public:

    enum Callback {
        CbReset = 0,
        CbSuspend = 1,
        CbResume = 2,
        CbSof = 3
    };

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

    // callbacks for each non-control endpoint (3)
    // and for each type of transaction (in, out, setup)
    static Usb::EpCallback endpointCallbacks[3][3];

    static inline void invokeUserEpCallback(uint8_t ep, Usb::Transaction t) {
        Usb::EpCallback cb = endpointCallbacks[ep][t];
        if (cb)
            cb(ep);
    }

    static const Usb::DeviceDescriptor *_dev;
    static const Usb::ConfigDescriptor *_conf;
    static const char **_strings;

    static uint16_t address;

    static Usb::Callback callbacks[4];
    static inline void invokeUserCallback(Callback cb) {
        Usb::Callback callback = callbacks[cb];
        if (callback)
            callback();
    }
};

#endif // USBD_H
