#ifndef THUNDERDRIVER_H
#define THUNDERDRIVER_H

#include "usb/usbdefs.h"

/**
 * A vendor specific driver for Thundercracker
 */
class ThunderDriver
{
public:
    ThunderDriver();

    void handleReset();
    void handleSuspend();
    void handleResume();
    void handleStartOfFrame();

    void onConfigComplete(uint16_t wValue);
    int controlRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    void inEndpointCallback(uint8_t ep, Usb::Transaction txn);
    void outEndpointCallback(uint8_t ep, Usb::Transaction txn);
};

#endif // THUNDERDRIVER_H
