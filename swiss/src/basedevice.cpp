#include "basedevice.h"

/*
 * iodevice is assumed to already be open/configured.
 */

BaseDevice::BaseDevice(IODevice &iodevice) :
    dev(iodevice)
{
}


UsbVolumeManager::VolumeOverviewReply *BaseDevice::getVolumeOverview(USBProtocolMsg &msg)
{
    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::VolumeOverview;

    if (!writeAndWaitForReply(msg)) {
        return 0;
    }

    if (msg.payloadLen() >= sizeof(UsbVolumeManager::VolumeOverviewReply)) {
        return msg.castPayload<UsbVolumeManager::VolumeOverviewReply>();
    }

    return 0;
}


const char *BaseDevice::getFirmwareVersion(USBProtocolMsg &msg)
{
    /*
     * Retrieve this base's firmware version.
     * Returns a pointer to the version info, which is captured in the
     * passed in buffer, or NULL on failure.
     */

    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::FirmwareVersion;

    if (!writeAndWaitForReply(msg)) {
        return "(unknown)";
    }

    msg.bytes[sizeof msg.bytes - 1] = 0;
    return msg.castPayload<char>();
}


UsbVolumeManager::VolumeDetailReply *BaseDevice::getVolumeDetail(USBProtocolMsg &msg, unsigned volBlockCode)
{
    /*
     * Retrieve details for a volume installed on this base.
     * Returns a pointer to the volume details, which is captured in the
     * passed in buffer, or NULL on failure.
     */

    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::VolumeDetail;
    msg.append((uint8_t*) &volBlockCode, sizeof volBlockCode);

    if (!writeAndWaitForReply(msg)) {
        return 0;
    }

    if (msg.payloadLen() >= sizeof(UsbVolumeManager::VolumeDetailReply)) {
        return msg.castPayload<UsbVolumeManager::VolumeDetailReply>();
    }

    return 0;
}


const UsbVolumeManager::SysInfoReply *BaseDevice::getBaseSysInfo(USBProtocolMsg &msg)
{
    /*
     * Retrieve this base's SysInfo.
     * Returns a pointer to the data, which is captured in the passed in buffer,
     * or NULL on failure.
     */

    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::BaseSysInfo;

    if (!writeAndWaitForReply(msg)) {
        return 0;
    }

    if (msg.payloadLen() >= sizeof(UsbVolumeManager::SysInfoReply)) {
        return msg.castPayload<UsbVolumeManager::SysInfoReply>();
    }

    return 0;
}


bool BaseDevice::pairCube(USBProtocolMsg &msg, uint64_t hwid, unsigned slot)
{
    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::PairCube;

    UsbVolumeManager::PairCubeRequest *req = msg.zeroCopyAppend<UsbVolumeManager::PairCubeRequest>();
    req->hwid = hwid;
    req->pairingSlot = slot;

    return writeAndWaitForReply(msg);
}


UsbVolumeManager::PairingSlotDetailReply *BaseDevice::pairingSlotDetail(USBProtocolMsg &msg, unsigned pairingSlot)
{
    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::PairingSlotDetail;
    msg.append((uint8_t*) &pairingSlot, sizeof pairingSlot);

    if (!writeAndWaitForReply(msg)) {
        return 0;
    }

    if (msg.payloadLen() >= sizeof(UsbVolumeManager::PairingSlotDetailReply)) {
        return msg.castPayload<UsbVolumeManager::PairingSlotDetailReply>();
    }

    return 0;
}


bool BaseDevice::waitForReply(uint32_t header, USBProtocolMsg &msg)
{
    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    msg.len = dev.readPacket(msg.bytes, msg.MAX_LEN);
    if (msg.header != header) {
        fprintf(stderr, "unexpected response. expecting 0x%x, got 0x%x\n", header, msg.header);
        return false;
    }

    return true;
}


bool BaseDevice::writeAndWaitForReply(USBProtocolMsg &msg)
{
    /*
     * Common helper.
     * Wait for data to become available, and ensure the response type
     * matches the expected type.
     */

    uint32_t headerToMatch = msg.header;
    dev.writePacket(msg.bytes, msg.len);

    return waitForReply(headerToMatch, msg);
}
