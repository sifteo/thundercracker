/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>
#include "usb/usbdefs.h"
#include "board.h"

class UsbDevice
{
public:
    static const uint16_t VendorID = 0x22FA;
#if (BOARD == BOARD_TEST_JIG)
    static const uint16_t ProductID = 0x0110;
#else
    static const uint16_t ProductID = 0x0105;
#endif

    static const uint8_t InEpAddr = 0x81;
    static const uint8_t OutEpAddr = 0x01;

    static const uint8_t OutEpMaxPacket = 64;

    static void init();

    static void handleReset();
    static void handleSuspend();
    static void handleResume();
    static void handleStartOfFrame();

    static void onConfigComplete(uint16_t wValue);
    static int controlRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len);

    static void inEndpointCallback(uint8_t ep);
    static void outEndpointCallback(uint8_t ep);

    static void handleINData(void *p);
    static void handleOUTData(void *p);

    static int read(uint8_t *buf, unsigned len);
    static int write(const uint8_t *buf, unsigned len);
};

#endif // USB_DEVICE_H
