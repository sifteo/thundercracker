#ifndef USBCONTROL_H
#define USBCONTROL_H

#include "usb/usbdefs.h"
#include "usb/usbhardware.h"

class UsbControl
{
public:
    static void setup();
    static void out();
    static void in();

private:
    UsbControl();

    static bool setupHandler();

    static void sendChunk();
    static int receiveChunk();

    static int requestDispatch(Usb::SetupData *req);

    static void setupRead(Usb::SetupData *req);
    static void setupWrite(Usb::SetupData *req);

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

#endif // USBCONTROL_H
