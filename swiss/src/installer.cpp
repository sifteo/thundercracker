#include "installer.h"
#include "usbprotocol.h"
#include "elfdebuginfo.h"
#include "progressbar.h"

#include <sifteo/abi/elf.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

int Installer::run(int argc, char **argv, IODevice &_dev)
{
    bool launcher = false;
    bool rpc = false;
    const char *path = NULL;

    for (unsigned i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-l")) {
            launcher = true;
        } else if (!strcmp(argv[i], "--rpc")) {
            rpc = true;
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr, "incorrect args\n");
            return 1;
        }
    }

    Installer installer(_dev);

    bool success = installer.install(path,
        IODevice::SIFTEO_VID, IODevice::BASE_PID, launcher, rpc);

    return success ? 0 : 1;
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
bool Installer::install(const char *path, int vid, int pid, bool launcher, bool rpc)
{
    isRPC = rpc;
    isLauncher = launcher;
    if (!launcher && !getPackageMetadata(path))
        return false;

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    if (!dev.open(vid, pid))
        return false;

    unsigned fileSize = getInstallableElfSize(f);
    if (!fileSize) {
        fprintf(stderr, "not a valid ELF file\n");
        return false;
    }

    if (launcher)
        fprintf(stderr, "updating launcher (%d bytes)\n", fileSize);
    else
        fprintf(stderr, "installing %s, version %s (%d bytes)\n",
            package.c_str(), version.c_str(), fileSize);

    if (!sendHeader(fileSize))
        return false;

    if (!sendFileContents(f, fileSize)) {
        return false;
    }

    if (!commit())
        return false;

    fclose(f);

    while (dev.numPendingOUTPackets()) {
        dev.processEvents();
    }
    return true;
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

bool Installer::sendHeader(uint32_t filesz)
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

    dev.writePacket(m.bytes, m.len);

    // wait for response that header was processed
    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::WroteHeaderOK) {
        fprintf(stderr, "not enough room for this app\n");
        return false;
    }

    return true;
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

    while (1) {
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

        dev.writePacket(m.bytes, m.len);
        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS)
            dev.processEvents();

        pb.update(progress);
    }
}

/*
 * We're done sending payload data - tell the master to commit this app.
 */
bool Installer::commit()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::WriteCommit;

    dev.writePacket(m.bytes, m.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);

    if ((m.header & 0xff) == UsbVolumeManager::WriteCommitOK) {
        fprintf(stderr, "successfully committed new volume\n");
        return true;
    }

    fprintf(stderr, "failed to write volume!\n");
    return false;
}
