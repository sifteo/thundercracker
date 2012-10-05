#include "manifest.h"
#include "usbprotocol.h"
#include "elfdebuginfo.h"
#include "tabularlist.h"
#include "metadata.h"

#include <sifteo/abi/elf.h>
#include <sifteo/abi/types.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iomanip>


int Manifest::run(int argc, char **argv, IODevice &_dev)
{
    bool rpc = false;
    
    for (unsigned i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rpc")) {
            rpc = true;
        } else {
            fprintf(stderr, "incorrect args\n");
            return 1;
        }
    }

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return 1;

    Manifest m(_dev, rpc);
    bool success = m.dumpBaseSysInfo() && m.getVolumeOverview() && m.dumpOverview() && m.dumpVolumes();

    return success ? 0 : 1;
}

Manifest::Manifest(IODevice &_dev, bool rpc) :
    dev(_dev), isRPC(rpc)
{}

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

const UsbVolumeManager::SysInfoReply *Manifest::getBaseSysInfo(USBProtocolMsg &buffer)
{
    buffer.init(USBProtocol::Installer);
    buffer.header |= UsbVolumeManager::BaseSysInfo;

    dev.writePacket(buffer.bytes, buffer.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    buffer.len = dev.readPacket(buffer.bytes, buffer.MAX_LEN);
    if ((buffer.header & 0xff) != UsbVolumeManager::BaseSysInfo) {
        fprintf(stderr, "unexpected response\n");
        return 0;
    }

    if (buffer.payloadLen() >= sizeof(UsbVolumeManager::SysInfoReply))
        return buffer.castPayload<UsbVolumeManager::SysInfoReply>();
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

bool Manifest::dumpBaseSysInfo()
{
    USBProtocolMsg buffer;
    const UsbVolumeManager::SysInfoReply *sysInfo = getBaseSysInfo(buffer);
    if (!sysInfo) {
        return false;
    }

    printf("Hardware ID: ");
    for (unsigned i = 0; i < sizeof(sysInfo->baseUniqueID); ++i) {
        printf("%x", sysInfo->baseUniqueID[i]);
    }
    printf("  Hardware Revision: %d\n", sysInfo->baseHwRevision);

    if (isRPC) {
        fprintf(stdout, "::hardware:");
        for (unsigned i = 0; i < sizeof(sysInfo->baseUniqueID); ++i) {
            fprintf(stdout, "%x", sysInfo->baseUniqueID[i]);
        }
        fprintf(stdout, ":%u\n", sysInfo->baseHwRevision);
    }

    return true;
}

bool Manifest::dumpOverview()
{
    USBProtocolMsg buffer;

    printf("System: %d kB  Free: %d kB  Firmware: %s\n",
        overview.systemBytes / 1024, overview.freeBytes / 1024,
        getFirmwareVersion(buffer));
        
    if (isRPC) {
        fprintf(stdout, "::firmware:%s\n", getFirmwareVersion(buffer)); fflush(stdout);
        fprintf(stdout, "::storage:%u:%u\n", overview.systemBytes, overview.freeBytes); fflush(stdout);
    }

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

    Metadata metadata(dev);

    // Volume table
    while (overview.bits.clearFirst(volBlockCode)) {
        USBProtocolMsg buffer;

        UsbVolumeManager::VolumeDetailReply *detail = getVolumeDetail(buffer, volBlockCode);
        if (!detail) {
            static UsbVolumeManager::VolumeDetailReply zero;
            detail = &zero;
        }
        // copy the type, since it appears to get overwritten between here and when
        // its needed for the RPC message
        uint16_t cpType = detail->type;

        std::string package = metadata.getString(volBlockCode, _SYS_METADATA_PACKAGE_STR);
        std::string version = metadata.getString(volBlockCode, _SYS_METADATA_VERSION_STR);
        std::string title   = metadata.getString(volBlockCode, _SYS_METADATA_TITLE_STR);

        table.cell() << std::setiosflags(std::ios::hex) << std::setw(2) << std::setfill('0') << volBlockCode;
        table.cell() << getVolumeTypeString(detail->type);
        table.cell(table.RIGHT) << (detail->selfBytes / 1024);
        table.cell(table.RIGHT) << (detail->childBytes / 1024);
        table.cell() << package;
        table.cell() << version;
        table.cell() << title;
        table.endRow();
        
        if (isRPC) {
            std::string packageRPC = package == "(none)" ? "" : package;
            std::string versionRPC = version == "(none)" ? "" : version;
            std::string titleRPC = title == "(none)" ? "" : title;
            
            fprintf(stdout, "::volume:%u:%u:%u:%u:%s:%s:%s\n",
                volBlockCode,
                cpType,
                detail->selfBytes,
                detail->childBytes,
                packageRPC.c_str(),
                versionRPC.c_str(),
                titleRPC.c_str());
            fflush(stdout);
        }
    }

    table.end();

    return true;
}
