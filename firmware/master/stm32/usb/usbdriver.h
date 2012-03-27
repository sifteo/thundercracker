#ifndef USBDRIVER_H
#define USBDRIVER_H

#include "usb/usbdefs.h"

/*
 * Generic interface for class level drivers.
 */
class UsbDriver
{
public:
    UsbDriver();    // don't implement

    static void handleReset();
    static void handleSuspend();
    static void handleResume();
    static void handleStartOfFrame();

    static void onConfigComplete(uint16_t wValue);
    static int controlRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static void inEndpointCallback(uint8_t ep);
    static void outEndpointCallback(uint8_t ep);

};

#endif // USBDRIVER_H
