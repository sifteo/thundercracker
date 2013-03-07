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

    bool beginLFSRestore(USBProtocolMsg &m, _SYSVolumeHandle vol, unsigned key, unsigned dataSize, uint32_t crc);

    const char *getFirmwareVersion(USBProtocolMsg &msg);
    const UsbVolumeManager::SysInfoReply *getBaseSysInfo(USBProtocolMsg &msg);

    UsbVolumeManager::VolumeOverviewReply *getVolumeOverview(USBProtocolMsg &msg);
    UsbVolumeManager::VolumeDetailReply *getVolumeDetail(USBProtocolMsg &msg, unsigned volBlockCode);
    UsbVolumeManager::LFSDetailReply *getLFSDetail(USBProtocolMsg &buffer, unsigned volBlockCode);

    bool pairCube(USBProtocolMsg &msg, uint64_t hwid, unsigned slot);
    UsbVolumeManager::PairingSlotDetailReply *pairingSlotDetail(USBProtocolMsg &msg, unsigned pairingSlot);

    bool requestReboot();

    bool getMetadata(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key);

    // helpers
    bool waitForReply(uint32_t header, USBProtocolMsg &msg);
    bool writeAndWaitForReply(USBProtocolMsg &msg);

private:
    IODevice &dev;
};

#endif // BASE_DEVICE_H
