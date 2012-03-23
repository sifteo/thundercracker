#ifndef USBCONTROL_H
#define USBCONTROL_H

#include "usb/usbdefs.h"

class UsbControl
{
public:
    UsbControl();

    static bool controlRequest(uint8_t ep, Usb::Transaction txn);

private:
    static void setup(uint8_t ea);
    static void out(uint8_t ea);
    static void in(uint8_t ea);

    static void sendChunk();
    static int receiveChunk();

    static int requestDispatch(Usb::SetupData *req);

    static void setupRead(Usb::SetupData *req);
    static void setupWrite(Usb::SetupData *req);

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

    struct ControlState {
        ControlStatus status;
        Usb::SetupData req;
        uint8_t *buf;
        uint16_t len;
        void (*complete)(Usb::SetupData *req);
    };
    static ControlState controlState;
};

#endif // USBCONTROL_H
