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
                     const Usb::WinUsbCompatIdHeaderDescriptor *win,
                     const Config & cfg);
    static void reset();

    // control requests
    static void setup();
    static void out();
    static void in();

private:
    static const Usb::DeviceDescriptor *devDesc;
    static const Usb::ConfigDescriptor *configDesc;
    static const Usb::WinUsbCompatIdHeaderDescriptor *winCompatDesc;

    static uint16_t address;
    static uint16_t _config;

    static bool setupHandler();
    static void setErrorState();

    enum ControlStatus {
        Idle,
        Stalled,
        DataIn,
        LastDataIn,
        StatusIn,
        DataOut,
        LastDataOut,
        StatusOut
    };

    enum DeviceState {
        Uninit      = 0,
        Stopped     = 1,
        Ready       = 2,
        Selected    = 3,
        Active      = 4
    };

    enum Ep0State {
        EP0_WAITING_SETUP,
        EP0_TX,
        EP0_WAITING_TX0,
        EP0_WAITING_STS,
        EP0_RX,
        EP0_SENDING_STS,
        EP0_ERROR
    };

    struct ControlState {
        Ep0State ep0Status;
        DeviceState state;
        uint16_t status;
        uint8_t configuration;
        Usb::SetupData req;
        uint8_t *pdata;
        uint16_t len;

        void ALWAYS_INLINE setupTransfer(const void *b, uint16_t sz) {
            len = sz;
            pdata = (uint8_t*)b;
        }
    };
    static ControlState controlState;
};

#endif // USBCORE_H
