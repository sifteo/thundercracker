#include "manifest.h"
#include "usbprotocol.h"
#include "elfdebuginfo.h"

#include <sifteo/abi/elf.h>
#include <sifteo/abi/types.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

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
    bool success = m.getVolumeOverview() && m.dumpVolumes();

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

bool Manifest::dumpVolumes()
{
    unsigned volBlockCode;

    fprintf(stderr, "%-4s %-8s %5s %5s %-30s %-10s %s\n",
        "Vol", "Type", "kbELF", "kbObj", "Package", "Version", "Title");

    while (overview.bits.clearFirst(volBlockCode)) {
        USBProtocolMsg buffer;

        UsbVolumeManager::VolumeDetailReply *detail = getVolumeDetail(buffer, volBlockCode);
        if (!detail) {
            static UsbVolumeManager::VolumeDetailReply zero;
            detail = &zero;
        }

        fprintf(stderr, "<%02x> %-8s %5d %5d ",
            volBlockCode,
            getVolumeTypeString(detail->type),
            detail->selfBytes / 1024, detail->childBytes / 1024);

        fprintf(stderr, "%-30s ", getMetadataString(buffer, volBlockCode, _SYS_METADATA_PACKAGE_STR));
        fprintf(stderr, "%-10s ", getMetadataString(buffer, volBlockCode, _SYS_METADATA_VERSION_STR));
        fprintf(stderr, "%s\n", getMetadataString(buffer, volBlockCode, _SYS_METADATA_TITLE_STR));
    }

    return true;
}