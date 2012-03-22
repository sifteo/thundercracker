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

    void inEndpointCallback(uint8_t ep, Usb::Transaction txn);
    void outEndpointCallback(uint8_t ep, Usb::Transaction txn);
};

#endif // THUNDERDRIVER_H
