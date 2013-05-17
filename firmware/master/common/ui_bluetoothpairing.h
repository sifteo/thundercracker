/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#ifndef _UI_BLUETOOTH_PAIRING
#define _UI_BLUETOOTH_PAIRING

#include <sifteo/abi.h>
#include "systime.h"

class UICoordinator;

/**
 * A modal popup which displays a Bluetooth pairing code.
 */

class UIBluetoothPairing {
public:
    UIBluetoothPairing(UICoordinator &uic, const char *code)
        : uic(uic), code(code) {}

    void init();

private:
    UICoordinator &uic;
    const char *code;

    void drawBackground();
    void drawDigit(unsigned addr, char digit);
};

#endif
