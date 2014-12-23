/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "manifest.h"
#include "usbprotocol.h"
#include "elfdebuginfo.h"
#include "tabularlist.h"
#include "metadata.h"
#include "swisserror.h"

#include <sifteo/abi/elf.h>
#include <sifteo/abi/types.h>

#include <stdio.h>
#include <string.h>
#include <iomanip>


int Manifest::run(int argc, char **argv, IODevice &_dev)
{
    bool rpc = false;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rpc")) {
            rpc = true;
        } else {
            fprintf(stderr, "incorrect args\n");
            return EINVAL;
        }
    }

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return ENODEV;
    }

    Manifest m(_dev, rpc);

    int rv = m.dumpBaseSysInfo();
    if (rv != EOK) {
        return rv;
    }

    rv = m.dumpOverview();
    if (rv != EOK) {
        return rv;
    }

    m.dumpVolumes();
    return EOK;
}

Manifest::Manifest(IODevice &_dev, bool rpc) :
    dev(_dev),
    base(_dev),
    isRPC(rpc)
{}


const char *Manifest::getVolumeTypeString(unsigned type)
{
    switch (type) {
        case _SYS_FS_VOL_LAUNCHER:  return "Launcher";
        case _SYS_FS_VOL_GAME:      return "Game";
    }

    snprintf(volTypeBuffer, sizeof volTypeBuffer, "%04x", type);
    return volTypeBuffer;
}


int Manifest::dumpBaseSysInfo()
{
    USBProtocolMsg buffer;
    const UsbVolumeManager::SysInfoReply *sysInfo = base.getBaseSysInfo(buffer);
    if (!sysInfo) {
        return EIO;
    }

    printf("Hardware ID: ");
    for (unsigned i = 0; i < sizeof(sysInfo->baseUniqueID); ++i) {
        printf("%02x", sysInfo->baseUniqueID[i]);
    }
    printf("  Hardware Revision: %d", sysInfo->baseHwRevision);
    if (sysInfo->sysVersion) {
        printf("  OS Version: 0x%06x", sysInfo->sysVersion);
    }
    printf("\n");

    if (isRPC) {

        /*
         * The base unique ID was previously printed with a %x formatter,
         * rather than %02x, which meant we could have non-uniform HWID string lengths.
         *
         * In order to allow for Sync to resolve the difference between any records
         * it may have created based on the old format, provide both the new and old
         * formats in RPC mode.
         */

        fprintf(stdout, "::hardware-id-bug:");
        for (unsigned i = 0; i < sizeof(sysInfo->baseUniqueID); ++i) {
            fprintf(stdout, "%x", sysInfo->baseUniqueID[i]);
        }
        fprintf(stdout, "\n");

        // and send the newer, correct format
        fprintf(stdout, "::hardware:");
        for (unsigned i = 0; i < sizeof(sysInfo->baseUniqueID); ++i) {
            fprintf(stdout, "%02x", sysInfo->baseUniqueID[i]);
        }
        fprintf(stdout, ":%u\n", sysInfo->baseHwRevision);

        if (sysInfo->sysVersion) {
            printf("::os-version:%u\n", sysInfo->sysVersion);
        }
    }

    return EOK;
}


int Manifest::dumpOverview()
{
    USBProtocolMsg buffer;

    if (UsbVolumeManager::VolumeOverviewReply *o = base.getVolumeOverview(buffer)) {
        overview = *o;
    } else {
        return EIO;
    }

    printf("System: %d kB  Free: %d kB  Firmware: %s\n",
        overview.systemBytes / 1024, overview.freeBytes / 1024,
        base.getFirmwareVersion(buffer));
        
    if (isRPC) {
        fprintf(stdout, "::firmware:%s\n", base.getFirmwareVersion(buffer)); fflush(stdout);
        fprintf(stdout, "::storage:%u:%u\n", overview.systemBytes, overview.freeBytes); fflush(stdout);
    }

    return EOK;
}

void Manifest::dumpVolumes()
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

        UsbVolumeManager::VolumeDetailReply *detail = base.getVolumeDetail(buffer, volBlockCode);
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
}
