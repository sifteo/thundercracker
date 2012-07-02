#include "manifest.h"
#include "usbprotocol.h"
#include "elfdebuginfo.h"
#include "tabularlist.h"

#include <sifteo/abi/elf.h>
#include <sifteo/abi/types.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iomanip>


int Manifest::run(int argc, char **argv, IODevice &_dev)
{
    if (argc != 1) {
        fprintf(stderr, "incorrect args\n");
        return 1;
    }

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached\n");
        return 1;
    }

    Manifest m(_dev);
    bool success = m.getVolumeOverview() && m.dumpOverview() && m.dumpVolumes();

    _dev.close();
    _dev.processEvents();

    return success ? 0 : 1;
}

Manifest::Manifest(IODevice &_dev) :
    dev(_dev)
{}

bool Manifest::getMetadata(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key)
{
    buffer.init(USBProtocol::Installer);
    buffer.header |= UsbVolumeManager::VolumeMetadata;
    UsbVolumeManager::VolumeMetadataRequest *req = buffer.zeroCopyAppend<UsbVolumeManager::VolumeMetadataRequest>();

    req->volume = volBlockCode;
    req->key = key;

    dev.writePacket(buffer.bytes, buffer.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    buffer.len = dev.readPacket(buffer.bytes, buffer.MAX_LEN);
    if ((buffer.header & 0xff) != UsbVolumeManager::VolumeMetadata) {
        fprintf(stderr, "unexpected response\n");
        return false;
    }

    return buffer.payloadLen() != 0;
}

const char *Manifest::getMetadataString(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key)
{
    if (!getMetadata(buffer, volBlockCode, key))
        return "(none)";

    buffer.bytes[sizeof buffer.bytes - 1] = 0;
    return buffer.castPayload<char>();
}

UsbVolumeManager::VolumeDetailReply *Manifest::getVolumeDetail(USBProtocolMsg &buffer, unsigned volBlockCode)
{
    buffer.init(USBProtocol::Installer);
    buffer.header |= UsbVolumeManager::VolumeDetail;
    buffer.append((uint8_t*) &volBlockCode, sizeof volBlockCode);

    dev.writePacket(buffer.bytes, buffer.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    buffer.len = dev.readPacket(buffer.bytes, buffer.MAX_LEN);
    if ((buffer.header & 0xff) != UsbVolumeManager::VolumeDetail) {
        fprintf(stderr, "unexpected response\n");
        return 0;
    }

    if (buffer.payloadLen() >= sizeof(UsbVolumeManager::VolumeDetailReply))
        return buffer.castPayload<UsbVolumeManager::VolumeDetailReply>();
    return 0;
}

const char *Manifest::getFirmwareVersion(USBProtocolMsg &buffer)
{
    buffer.init(USBProtocol::Installer);
    buffer.header |= UsbVolumeManager::FirmwareVersion;

    dev.writePacket(buffer.bytes, buffer.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    buffer.len = dev.readPacket(buffer.bytes, buffer.MAX_LEN);
    if ((buffer.header & 0xff) != UsbVolumeManager::FirmwareVersion) {
        fprintf(stderr, "unexpected response\n");
        return "(unknown)";
    }

    buffer.bytes[sizeof buffer.bytes - 1] = 0;
    return buffer.castPayload<char>();
}

bool Manifest::getVolumeOverview()
{
    USBProtocolMsg m(USBProtocol::Installer);

    m.header |= UsbVolumeManager::VolumeOverview;
    dev.writePacket(m.bytes, m.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::VolumeOverview) {
        fprintf(stderr, "unexpected response\n");
        return false;
    }

    overview = *m.castPayload<UsbVolumeManager::VolumeOverviewReply>();
    return true;
}

const char *Manifest::getVolumeTypeString(unsigned type)
{
    switch (type) {
        case _SYS_FS_VOL_LAUNCHER:  return "Launcher";
        case _SYS_FS_VOL_GAME:      return "Game";
    }

    snprintf(volTypeBuffer, sizeof volTypeBuffer, "%04x", type);
    return volTypeBuffer;
}

bool Manifest::dumpOverview()
{
    USBProtocolMsg buffer;

    printf("System: %d kB  Free: %d kB  Firmware: %s\n",
        overview.systemBytes / 1024, overview.freeBytes / 1024,
        getFirmwareVersion(buffer));

    return true;
}

bool Manifest::dumpVolumes()
{
    unsigned volBlockCode;
    TabularList table;

    // Heading
    table.cell() << "VOL";
    table.cell() << "TYPE";
    table.cell() << "ELF-KB";
    table.cell() << "OBJ-KB";
    table.cell() << "PACKAGE";
    table.cell() << "VERSION";
    table.cell() << "TITLE";
    table.endRow();

    // Volume table
    while (overview.bits.clearFirst(volBlockCode)) {
        USBProtocolMsg buffer;

        UsbVolumeManager::VolumeDetailReply *detail = getVolumeDetail(buffer, volBlockCode);
        if (!detail) {
            static UsbVolumeManager::VolumeDetailReply zero;
            detail = &zero;
        }

        table.cell() << std::setiosflags(std::ios::hex) << std::setw(2) << std::setfill('0') << volBlockCode;
        table.cell() << getVolumeTypeString(detail->type);
        table.cell(table.RIGHT) << (detail->selfBytes / 1024);
        table.cell(table.RIGHT) << (detail->childBytes / 1024);
        table.cell() << getMetadataString(buffer, volBlockCode, _SYS_METADATA_PACKAGE_STR);
        table.cell() << getMetadataString(buffer, volBlockCode, _SYS_METADATA_VERSION_STR);
        table.cell() << getMetadataString(buffer, volBlockCode, _SYS_METADATA_TITLE_STR);
        table.endRow();
    }

    table.end();

    return true;
}