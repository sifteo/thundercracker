/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "board.h"
#include "systime.h"
#include "tasks.h"
#include "usb/usbdevice.h"
#include "bootloader.h"
#include "powermanager.h"

/*
 * Bootloader specific entry point.
 * Lower level init happens in setup.cpp.
 */
void bootloadMain(bool userRequestedUpdate)
{
    PowerManager::init();

    Bootloader loader;
    loader.init();
    loader.exec(userRequestedUpdate); // never returns
}
