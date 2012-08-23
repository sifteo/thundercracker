#include "backup.h"
#include "usbprotocol.h"
#include "progressbar.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
#   define unlink(x) _unlink(x)
#else
#   include <unistd.h>
#endif


int Backup::run(int argc, char **argv, IODevice &_dev)
{
    const char *path = NULL;

    if (argc != 2) {
        fprintf(stderr, "incorrect args\n");
        return 1;
    }

    Backup m(_dev);
    return !m.backup(argv[1]);
}

Backup::Backup(IODevice &_dev) : dev(_dev) {}

bool Backup::backup(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return false;

    requestProgress = 0;
    replyProgress = 0;

    bool success = writeFileHeader(f) && writeFlashContents(f);
    fclose(f);

    if (!success)
        unlink(path);

    return success;
}

bool Backup::writeFileHeader(FILE *f)
{
    /*
     * This is a basic header that describes the geometry of the flash device,
     * and identifies this file as something that Siftulator knows how to import
     * with the -F command line option.
     */

    struct Header {
        uint64_t    magic;
        uint32_t    version;
        uint32_t    fileSize;
        uint32_t    cube_count;
        uint32_t    cube_nvmSize;
        uint32_t    cube_extSize;
        uint32_t    cube_sectorSize;
        uint32_t    mc_pageSize;
        uint32_t    mc_blockSize;
        uint32_t    mc_capacity;
        uint32_t    uniqueID;
        uint32_t    reserved[52];
    };

    static const Header hdr = {
        0x534c467974666953LLU,      // magic
        1,                          // version
        DEVICE_SIZE + PAGE_SIZE,    // fileSize
        0, 0, 0, 0,                 // (no cubes)
        PAGE_SIZE,                  // mc_pageSize
        BLOCK_SIZE,                 // mc_blockSize,
        DEVICE_SIZE,                // mc_capacity
        -1,                         // uniqueID,
    };

    STATIC_ASSERT(sizeof hdr == PAGE_SIZE);
    return fwrite(&hdr, sizeof hdr, 1, f) == 1;
}

bool Backup::sendRequest()
{
    unsigned offset = requestProgress;
    if (offset >= DEVICE_SIZE)
        return false;

    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::FlashDeviceRead;
    UsbVolumeManager::FlashDeviceReadRequest *req =
        m.zeroCopyAppend<UsbVolumeManager::FlashDeviceReadRequest>();

    req->address = offset;
    req->length = std::min(DEVICE_SIZE - offset,
        USBProtocolMsg::MAX_LEN - USBProtocolMsg::HEADER_BYTES);

    requestProgress = offset + req->length;

    dev.writePacket(m.bytes, m.len);

    return true;
}

bool Backup::writeReply(FILE *f)
{
    USBProtocolMsg m;

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::FlashDeviceRead) {
        fprintf(stderr, "\nunexpected response\n");
        return false;
    }

    unsigned len = m.payloadLen();
    replyProgress += len;

    return fwrite(m.castPayload<uint8_t>(), len, 1, f) == 1;
}

bool Backup::writeFlashContents(FILE *f)
{
    ScopedProgressBar pb(DEVICE_SIZE);

    // Queue up the first few reads, respond as results arrive
    for (unsigned i = 0; i < 3; ++i)
        sendRequest();

    while (1) {
        dev.processEvents();

        while (dev.numPendingINPackets() != 0) {
            if (!writeReply(f))
                return false;

            pb.update(replyProgress);
            sendRequest();
            if (replyProgress == DEVICE_SIZE)
                return true;
        }
    }
}
