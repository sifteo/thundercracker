/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usbprotocol.h"
#include "flash_device.h"
#include "usbvolumemanager.h"
#include "macros.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#include "hardware.h"
#include "factorytest.h"
#include "sampleprofiler.h"
#endif

/*
 * Table of USB subsystem handlers.
 * Order in this table must match the SubSystem enum.
 */
const USBProtocol::SubSystemHandler USBProtocol::subsystemHandlers[] = {
    UsbVolumeManager::onUsbData,            // 0
    #if (defined(SIFTEO_SIMULATOR) || (BOARD == BOARD_TEST_JIG) || defined(RFTEST))
    0,
    0
    #else
    FactoryTest::usbHandler,                // 1
    SampleProfiler::onUSBData               // 2
    #endif
};

void USBProtocol::dispatch(const USBProtocolMsg &m)
{
    unsigned subsys = static_cast<unsigned>(m.subsystem());
    if (subsys < arraysize(subsystemHandlers)) {
        subsystemHandlers[subsys](m);
    }
}
