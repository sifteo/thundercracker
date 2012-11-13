#ifndef BASE_DEVICE_H
#define BASE_DEVICE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

/*
 * Represents the Sifteo Base.
 * Handles requests/responses from the base over the wire.
 */

class BaseDevice
{
public:
    BaseDevice(IODevice &iodevice);

    UsbVolumeManager::VolumeOverviewReply *getVolumeOverview(USBProtocolMsg &msg);
    const char *getFirmwareVersion(USBProtocolMsg &msg);
    UsbVolumeManager::VolumeDetailReply *getVolumeDetail(USBProtocolMsg &msg, unsigned volBlockCode);
    const UsbVolumeManager::SysInfoReply *getBaseSysInfo(USBProtocolMsg &msg);

    bool pairCube(USBProtocolMsg &msg, uint64_t hwid, unsigned slot);
    UsbVolumeManager::PairingSlotDetailReply *pairingSlotDetail(USBProtocolMsg &msg, unsigned pairingSlot);

    bool requestReboot();

    // helpers
    bool waitForReply(uint32_t header, USBProtocolMsg &msg);
    bool writeAndWaitForReply(USBProtocolMsg &msg);

private:
    IODevice &dev;
};

#endif // BASE_DEVICE_H
