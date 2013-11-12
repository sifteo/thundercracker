/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usbprotocol.h"
#include "flash_device.h"
#include "usbvolumemanager.h"
#include "macros.h"
#include "event.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#include "hardware.h"
#include "factorytest.h"
#include "sampleprofiler.h"
#endif

USBProtocol USBProtocol::instance;

bool USBProtocol::setUserQueues(SvmMemory::VirtAddr send, SvmMemory::VirtAddr receive)
{
    return instance.userSendQueue.attach(send) && instance.userReceiveQueue.attach(receive);
}

void USBProtocol::dispatch(const USBProtocolMsg &m)
{
    switch (m.subsystem()) {
        case Installer:     UsbVolumeManager::onUsbData(m); return;
    #if (!(defined(SIFTEO_SIMULATOR) || ((BOARD == BOARD_TEST_JIG) && !defined(BOOTLOADER)) || defined(RFTEST)))
        case FactoryTest:   FactoryTest::usbHandler(m); return;
        case Profiler:      SampleProfiler::onUSBData(m); return;
    #endif
        case User:          onReceiveData(m); return;
        default:
            return;
    }
}

void USBProtocol::onReceiveData(const USBProtocolMsg &m)
{
    int payloadLen = m.payloadLen();

    USBProtocol::instance.counters.rxPackets++;
    USBProtocol::instance.counters.rxBytes += payloadLen;

    /*
     * Is there a user pipe with room for this packet?
     */

    UsbQueue &queue = USBProtocol::instance.userReceiveQueue;
    if (queue.hasQueue() && queue.writeAvailable()) {

        _SYSUsbPacket *dest = queue.reserve();
        dest->length = payloadLen;
        dest->type = m.header;
        memcpy(dest->bytes, m.castPayload<uint8_t>(), payloadLen);

        // Notify userspace that some data has arrived.
        queue.commit();
        return Event::setBasePending(Event::PID_BASE_USB_READ_AVAILABLE);
    }

    // Dropped a user packet on the floor!
    USBProtocol::instance.counters.rxUserDropped++;
}
