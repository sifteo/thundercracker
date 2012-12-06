#include "basedevice.h"

/*
 * iodevice is assumed to already be open/configured.
 */

BaseDevice::BaseDevice(IODevice &iodevice) :
    dev(iodevice)
{
}


bool BaseDevice::beginLFSRestore(USBProtocolMsg &m, unsigned vol, unsigned key, unsigned dataSize, uint32_t crc)
{
    m.init(USBProtocol::Installer);
    m.header |= UsbVolumeManager::WriteLFSObjectHeader;

    UsbVolumeManager::LFSObjectHeader *req = m.zeroCopyAppend<UsbVolumeManager::LFSObjectHeader>();
    req->vh = vol;
    req->crc = crc;
    req->key = key;
    req->dataSize = dataSize;

    return writeAndWaitForReply(m);
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


UsbVolumeManager::LFSDetailReply *BaseDevice::getLFSDetail(USBProtocolMsg &msg, unsigned volBlockCode)
{
    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::LFSDetail;
    msg.append((uint8_t*) &volBlockCode, sizeof volBlockCode);

    if (!writeAndWaitForReply(msg)) {
        return 0;
    }

    if (msg.payloadLen() >= sizeof(UsbVolumeManager::LFSDetailReply)) {
        return msg.castPayload<UsbVolumeManager::LFSDetailReply>();
    }

    return 0;
}


bool BaseDevice::pairCube(USBProtocolMsg &msg, uint64_t hwid, unsigned slot)
{
    /*
     * Add a new pairing record for the given HWID in the given slot.
     * This unconditionally overwrites a HWID already in that slot.
     */

    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::PairCube;

    UsbVolumeManager::PairCubeRequest *req = msg.zeroCopyAppend<UsbVolumeManager::PairCubeRequest>();
    req->hwid = hwid;
    req->pairingSlot = slot;

    return writeAndWaitForReply(msg);
}


UsbVolumeManager::PairingSlotDetailReply *BaseDevice::pairingSlotDetail(USBProtocolMsg &msg, unsigned pairingSlot)
{
    /*
     * Retrieve pairing slot detail for the requested slot, which is captured
     * in the passed in buffer, or NULL on failure.
     */

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


bool BaseDevice::requestReboot()
{
    USBProtocolMsg m(USBProtocol::FactoryTest);
    m.append(12);   // reboot request command
    dev.writePacket(m.bytes, m.len);
    return true;
}


bool BaseDevice::getMetadata(USBProtocolMsg &msg, unsigned volBlockCode, unsigned key)
{
    msg.init(USBProtocol::Installer);
    msg.header |= UsbVolumeManager::VolumeMetadata;
    UsbVolumeManager::VolumeMetadataRequest *req = msg.zeroCopyAppend<UsbVolumeManager::VolumeMetadataRequest>();

    req->volume = volBlockCode;
    req->key = key;

    if (!writeAndWaitForReply(msg)) {
        return false;
    }

    return msg.payloadLen() != 0;
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
    if (dev.writePacket(msg.bytes, msg.len) != msg.len) {
        return false;
    }

    return waitForReply(headerToMatch, msg);
}
