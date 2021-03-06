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

#include "installer.h"
#include "usbprotocol.h"
#include "usbvolumemanager.h"
#include "elfdebuginfo.h"
#include "progressbar.h"
#include "util.h"
#include "basedevice.h"
#include "swisserror.h"

#include <sifteo/abi/elf.h>

#include <stdio.h>
#include <string.h>

int Installer::run(int argc, char **argv, IODevice &_dev)
{
    bool launcher = false;
    bool forceLauncher = false;
    bool rpc = false;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-l")) {
            launcher = true;
        } else if (!strcmp(argv[i], "--rpc")) {
            rpc = true;
        } else if (!strcmp(argv[i], "-f")) {
            forceLauncher = true;
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr, "incorrect args\n");
            return EINVAL;
        }
    }

    Installer installer(_dev);
    return installer.install(path,
                             IODevice::SIFTEO_VID,
                             IODevice::BASE_PID,
                             launcher,
                             forceLauncher,
                             rpc);
}

Installer::Installer(IODevice &_dev) :
    dev(_dev)
{}

/*
 * High level program flow for installation.
 *
 * - Ensure given package has the required metatdata.
 * - Send the header info - device may take a while to process this as it
 *      erases/allocates space for upcoming transfer
 * - Send the content of the application.
 * - Commit the transaction.
 */
int Installer::install(const char *path, int vid, int pid, bool launcher, bool forceLauncher, bool rpc)
{
    isRPC = rpc;
    isLauncher = launcher;
    if (!launcher && !getPackageMetadata(path)) {
        return EINVAL;
    }

    if (launcher && !forceLauncher) {
        /*
         * Enforce the convention that the filename at least starts with 'launcher'
         * Helps protect against people accidentally installing unintended games,
         * or even unencrypted firmware, as the launcher.
         */

        const char *prefix = "launcher";
        if (strncmp(Util::filepathBase(path), prefix, strlen(prefix)) != 0) {
            puts("this doesn't look like a launcher. use the -f option to force install it.");
            return EINVAL;
        }
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return ENOENT;
    }

    if (!dev.open(vid, pid)) {
        return ENODEV;
    }

    unsigned fileSize = getInstallableElfSize(f);
    if (!fileSize) {
        fprintf(stderr, "not a valid ELF file\n");
        return EINVAL;
    }

    if (launcher)
        printf("updating launcher (%d bytes)\n", fileSize);
    else
        printf("installing %s, version %s (%d bytes)\n",
            package.c_str(), version.c_str(), fileSize);

    int rv = sendHeader(fileSize);
    if (rv != 0) {
        return rv;
    }

    bool success = sendFileContents(f, fileSize) && commit();
    if (!success) {
        return EIO;
    }

    return EOK;
}

/*
 * Return the size, in bytes, of the installable portion of this ELF.
 * This excludes any debug sections, if any. If the file is not valid,
 * returns zero.
 *
 * This works by looking for the end of the last program segment.
 */
unsigned Installer::getInstallableElfSize(FILE *f)
{
    Elf::FileHeader fh;
    Elf::ProgramHeader ph;
    unsigned size = 0;

    rewind(f);
    if (fread(&fh, sizeof fh, 1, f) < 1)
        return 0;

    if (fh.e_ident[0] != Elf::Magic0 || fh.e_ident[1] != Elf::Magic1 ||
        fh.e_ident[2] != Elf::Magic2 || fh.e_ident[3] != Elf::Magic3)
        return 0;

    for (unsigned i = 0; i < fh.e_phnum; ++i) {
        fseek(f, fh.e_phoff + i * fh.e_phentsize, SEEK_SET);
        if (fread(&ph, sizeof ph, 1, f) < 1)
            return 0;
        size = std::max<unsigned>(size, ph.p_offset + ph.p_filesz);
    }

    return size;
}

/*
 * Read package metadata from the elf for this application.
 * For now, must include package and version strings.
 */
bool Installer::getPackageMetadata(const char *path)
{
    ELFDebugInfo dbgInfo;
    if (!dbgInfo.init(path)) {
        fprintf(stderr, "couldn't open %s: %s\n", path, strerror(errno));
        return false;
    }

    if (!dbgInfo.metadataString(_SYS_METADATA_PACKAGE_STR, package)) {
        fprintf(stderr, "couldn't find package string - ensure you've included a call to Metadata::package() in your application\n");
        return false;
    }

    if (!dbgInfo.metadataString(_SYS_METADATA_VERSION_STR, version)) {
        fprintf(stderr, "couldn't find version - ensure you've included a call to Metadata::package() in your application\n");
        return false;
    }

    return true;
}

int Installer::sendHeader(uint32_t filesz)
{
    USBProtocolMsg m(USBProtocol::Installer);

    if (isLauncher) {
        m.header |= UsbVolumeManager::WriteLauncherHeader;
        m.append((uint8_t*)&filesz, sizeof filesz);
    } else {
        m.header |= UsbVolumeManager::WriteGameHeader;
        m.append((uint8_t*)&filesz, sizeof filesz);
        if (package.size() + 1 > m.bytesFree()) {
            fprintf(stderr, "package name too long\n");
            return false;
        }
        m.append((uint8_t*)package.c_str(), package.size());
    }

    if (dev.writePacket(m.bytes, m.len) < 0) {
        return EIO;
    }

    bool success = BaseDevice(dev).waitForReply(UsbVolumeManager::WroteHeaderOK, m);
    if (!success) {
        if (m.header == UsbVolumeManager::WroteHeaderFail) {
            fprintf(stderr, "error: not enough room for this app\n");
            return ENOSPC;
        } else {
            fprintf(stderr, "error: unexpected response (0x%x)\n", m.header);
            return EIO;
        }
    }

    return EOK;
}

/*
 * Write the body of this file.
 *
 * There are no restrictions on the format of the payload - just fit as much
 * into each packet as we can.
 */
bool Installer::sendFileContents(FILE *f, uint32_t filesz)
{
    rewind(f);
 
    unsigned progress = 0;
    ScopedProgressBar pb(filesz);

    for (;;) {
        USBProtocolMsg m(USBProtocol::Installer);
        m.header |= UsbVolumeManager::WritePayload;

        unsigned chunk = std::min(filesz - progress, m.bytesFree());
        m.len += chunk;
        progress += chunk;
        if (isRPC) {
            fprintf(stdout, "::progress:%u:%u\n", progress, filesz); fflush(stdout);
        }
        if (!chunk)
            return true;

        if (fread(m.payload, chunk, 1, f) != 1) {
            fprintf(stderr, "read error\n");
            return false;
        }

        if (dev.writePacket(m.bytes, m.len) < 0) {
            return false;
        }

        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS) {
            dev.processEvents(1);
        }

        pb.update(progress);
    }

    // should never get here
    return false;
}

/*
 * We're done sending payload data - tell the master to commit this app.
 */
bool Installer::commit()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::WriteCommit;

    if (dev.writePacket(m.bytes, m.len) < 0) {
        return false;
    }

    BaseDevice baseDevice(dev);
    if (!baseDevice.waitForReply(UsbVolumeManager::WriteCommitOK, m)) {
        fprintf(stderr, "failed to write volume!\n");
        return false;
    }

    if (m.payloadLen() >= 1) {
        uint8_t volumeBlockCode = m.payload[0];
        printf("successfully committed new volume 0x%x\n", volumeBlockCode);
        if (isRPC) {
            fprintf(stdout, "::volume:%u\n", volumeBlockCode); fflush(stdout);
        }
    } else {
        printf("successfully committed new volume\n");
    }

    return true;
}
