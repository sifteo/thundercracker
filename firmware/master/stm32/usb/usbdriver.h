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

    static void setConfig(uint16_t wValue);

    static void inEndpointCallback(uint8_t ep, Usb::Transaction txn);
    static void outEndpointCallback(uint8_t ep, Usb::Transaction txn);

};

#endif // USBDRIVER_H
