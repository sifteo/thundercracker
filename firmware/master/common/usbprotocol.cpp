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

void USBProtocol::dispatch(const USBProtocolMsg &m)
{
    switch (m.subsystem()) {
        case Installer:     UsbVolumeManager::onUsbData(m); return;
    #if (!(defined(SIFTEO_SIMULATOR) || ((BOARD == BOARD_TEST_JIG) && !defined(BOOTLOADER)) || defined(RFTEST)))
        case FactoryTest:   FactoryTest::usbHandler(m); return;
        case Profiler:      SampleProfiler::onUSBData(m); return;
    #endif
        default:
            return;
    }
}
