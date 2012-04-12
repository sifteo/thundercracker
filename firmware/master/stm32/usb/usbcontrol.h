#ifndef USBCONTROL_H
#define USBCONTROL_H

#include "usb/usbdefs.h"

class UsbControl
{
public:
    UsbControl();

    static bool controlRequest(uint8_t ep, Usb::Transaction txn);

private:
    static void setup();
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
        uint8_t *pdata;
        uint16_t len;
        // this buffer is mostly used for setup data, but also buffers string
        // descriptors, so its size currently serves as the upper limit on those
        uint8_t buf[32];
    };
    static ControlState controlState;
};

#endif // USBCONTROL_H
