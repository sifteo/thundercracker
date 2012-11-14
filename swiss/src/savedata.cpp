#include "savedata.h"
#include "basedevice.h"
#include "util.h"
#include "bits.h"
#include "metadata.h"
#include "progressbar.h"

#include <errno.h>
#include <string>
#include <stdlib.h>

int SaveData::run(int argc, char **argv, IODevice &_dev)
{
    bool success = false;
    bool rpc = false;
    SaveData saveData(_dev);

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return 1;

    if (argc >= 4 && !strcmp(argv[1], "extract")) {
        int ipath = 3;
        int ivol = 2;
        
        if (!strcmp(argv[2], "--rpc")) {
            ipath = 4;
            ivol = 3;
            rpc = true;
        }

        unsigned volume;
        const char *path = argv[ipath];
        success = Util::parseVolumeCode(argv[ivol], volume) && saveData.extract(volume, path, rpc);

    } else if (argc >= 3 && !strcmp(argv[1], "restore")) {

        const char *path = argv[2];
        success = saveData.restore(path);

    } else {
        fprintf(stderr, "incorrect args\n");
    }

    return success ? 0 : 1;
}

SaveData::SaveData(IODevice &_dev) :
    dev(_dev)
{}

bool SaveData::extract(unsigned volume, const char *filepath, bool rpc)
{
    /*
     * Retrieve all LFS volumes for a given parent volume.
     *
     * Concatenate their data into the file @ filepath.
     *
     * We don't do any parsing of the data at this point.
     */

    USBProtocolMsg buf;
    BaseDevice base(dev);
    UsbVolumeManager::LFSDetailReply *reply = base.getLFSDetail(buf, volume);
    if (!reply) {
        return false;
    }

    if (reply->count == 0) {
        printf("no savedata found for volume 0x%x\n", volume);
        return true;
    }

    FILE *fout = fopen(filepath, "wb");
    if (!fout) {
        fprintf(stderr, "couldn't open %s: %s\n", filepath, strerror(errno));
        return false;
    }

    if (!writeFileHeader(fout, volume, reply->count)) {
        return false;
    }

    return writeVolumes(reply, fout, rpc);
}


bool SaveData::restore(const char *filepath)
{
    /*
     * please implement me. thank you in advance.
     */

    FILE *fin = fopen(filepath, "rb");
    if (!fin) {
        fprintf(stderr, "couldn't open %s: %s\n", filepath, strerror(errno));
        return false;
    }

    struct MiniHeader {
        uint64_t    magic;
        uint32_t    version;
    } minihdr;

    if (fread(&minihdr, sizeof minihdr, 1, fin) != 1) {
        fprintf(stderr, "i/o error: %s\n", strerror(errno));
        fclose(fin);
        return false;
    }

    if (minihdr.magic != MAGIC) {
        fprintf(stderr, "%s is not a recognized savedata file\n", filepath);
        fclose(fin);
        return false;
    }

    rewind(fin);

    bool rv;

    switch (minihdr.version) {
    case 0x1:
        rv = restoreV1(fin);
        break;
    case 0x2:
        rv = restoreV2(fin);
        break;
    default:
        fprintf(stderr, "unsupported savedata file version: 0x%x\n", minihdr.version);
        rv = false;
    }

    fclose(fin);
    return rv;
}


static bool readStr(std::string &s, FILE *f)
{
    /*
     * strings are preceded by their uint32_t length.
     * a little stupid to allocate and free the intermediate buffer, but
     * handy to be able to return a std::string.
     */

    uint32_t length;

    if (fread(&length, sizeof length, 1, f) != 1) {
        return false;
    }

    char *buf = (char*)malloc(length);
    if (!buf) {
        return false;
    }

    if (fread(buf, length, 1, f) != 1) {
        return false;
    }

    s.assign(buf, length);
    free(buf);

    return true;
}


bool SaveData::volumeCodeForPackage(const std::string & pkg, unsigned &volBlockCode)
{
    /*
     * Search the installed volumes for the given package string, and retrieve
     * its current volume code if it exists.
     */

    BaseDevice base(dev);
    Metadata metadata(dev);

    USBProtocolMsg m;

    UsbVolumeManager::VolumeOverviewReply *overview = base.getVolumeOverview(m);
    if (!overview) {
        return false;
    }

    while (overview->bits.clearFirst(volBlockCode)) {
        if (pkg == metadata.getString(volBlockCode, _SYS_METADATA_PACKAGE_STR)) {
            return true;
        }
    }

    return false;
}


bool SaveData::restoreV1(FILE *f)
{
    fprintf(stderr, "restoring savedata file version 0x1\n");

    HeaderV1 hdr;
    if (fread(&hdr, sizeof hdr, 1, f) != 1) {
        fprintf(stderr, "i/o error: %s\n", strerror(errno));
        return false;
    }

    return false;
}


bool SaveData::restoreV2(FILE *f)
{
    HeaderV2 hdr;
    if (fread(&hdr, sizeof hdr, 1, f) != 1) {
        fprintf(stderr, "i/o error: %s\n", strerror(errno));
        return false;
    }

    std::string packageStr, versionStr;
    if (!readStr(packageStr, f) || !readStr(versionStr, f)) {
        return false;
    }

#if 0
    fprintf(stderr, "restoring savedata file version 0x2\n");

    printf("version: 0x%x\n", hdr.version);
    printf("numBlocks: 0x%x\n", hdr.numBlocks);
    printf("mc_pageSize: 0x%x\n", hdr.mc_pageSize);
    printf("mc_blockSize: 0x%x\n", hdr.mc_blockSize);

    printf("baseUniqueID: ");
    for (unsigned i = 0; i < sizeof(hdr.baseUniqueID); ++i) {
        printf("%02x", hdr.baseUniqueID[i]);
    }
    printf("\n");

    printf("baseHwRevision: 0x%x\n", hdr.baseHwRevision);
    printf("packageStr: %s\n", packageStr.c_str());
    printf("versionStr: %s\n", versionStr.c_str());
#endif

    return false;
}


bool SaveData::extractLFSVolume(unsigned address, unsigned len, FILE *f)
{
    fprintf(stderr, "extracing volume at address 0x%x, len 0x%x\n", address, len);
    return false;
}


static bool writeStr(const std::string &s, FILE *f)
{
    uint32_t length = s.length();

    if (fwrite(&length, sizeof length, 1, f) != 1)
        return false;

    if (fwrite(s.c_str(), length, 1, f) != 1)
        return false;

    return true;
}


bool SaveData::writeFileHeader(FILE *f, unsigned volBlockCode, unsigned numVolumes)
{
    /*
     * Simple header that describes the contents of our savedata file.
     * Subset of the info specified in a Backup.
     */

    HeaderV2 hdr = {
        MAGIC,          // magic
        VERSION,        // version
        numVolumes,     // numVolumes
        PAGE_SIZE,      // mc_pageSize
        BLOCK_SIZE,     // mc_blockSize,
    };

    BaseDevice base(dev);
    USBProtocolMsg m;
    const UsbVolumeManager::SysInfoReply *sysinfo = base.getBaseSysInfo(m);
    if (!sysinfo) {
        return false;
    }

    memcpy(hdr.baseUniqueID, sysinfo->baseUniqueID, sizeof(hdr.baseUniqueID));
    hdr.baseHwRevision = sysinfo->baseHwRevision;


    Metadata metadata(dev);
    std::string packageID = metadata.getString(volBlockCode, _SYS_METADATA_PACKAGE_STR);
    std::string version = metadata.getString(volBlockCode, _SYS_METADATA_VERSION_STR);
    int uuidLen = metadata.getBytes(volBlockCode, _SYS_METADATA_UUID, hdr.appUUID.bytes, sizeof hdr.appUUID.bytes);

    if (packageID.empty() || version.empty() || uuidLen <= 0) {
        return false;
    }

    if (fwrite(&hdr, sizeof hdr, 1, f) != 1)
        return false;

    return writeStr(packageID, f) && writeStr(version, f);
}

bool SaveData::writeVolumes(UsbVolumeManager::LFSDetailReply *reply, FILE *f, bool rpc)
{
    ScopedProgressBar pb(reply->count * BLOCK_SIZE);
    unsigned progTotal = reply->count * BLOCK_SIZE;
    unsigned overallProgress = 0;

    /*
     * For each block, request all of its data, and write it out to our file.
     */

    for (unsigned i = 0; i < reply->count; ++i) {

        unsigned baseAddress = reply->records[i].address;
        unsigned requestProgress = 0, replyProgress = 0;

        // Queue up the first few reads, respond as results arrive
        for (unsigned b = 0; b < 3; ++b) {
            sendRequest(baseAddress, requestProgress);
        }

        while (replyProgress < BLOCK_SIZE) {
            dev.processEvents();

            while (dev.numPendingINPackets() != 0) {
                if (!writeReply(f, replyProgress)) {
                    return false;
                }

                pb.update(overallProgress + replyProgress);
                if (rpc) {
                    fprintf(stdout, "::progress:%u:%u\n", overallProgress + replyProgress, progTotal); fflush(stdout);
                }
                sendRequest(baseAddress, requestProgress);
            }
        }
        overallProgress += BLOCK_SIZE;
    }

    return true;
}


bool SaveData::sendRequest(unsigned baseAddr, unsigned & progress)
{
    /*
     * Request another chunk of data, maintaining progress within
     * the current block.
     */

    unsigned offset = progress;
    if (offset >= BLOCK_SIZE) {
        return false;
    }

    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::FlashDeviceRead;
    UsbVolumeManager::FlashDeviceReadRequest *req =
        m.zeroCopyAppend<UsbVolumeManager::FlashDeviceReadRequest>();

    req->address = baseAddr + offset;
    req->length = std::min(BLOCK_SIZE - offset,
        USBProtocolMsg::MAX_LEN - USBProtocolMsg::HEADER_BYTES);

    progress = offset + req->length;

    dev.writePacket(m.bytes, m.len);

    return true;
}


bool SaveData::writeReply(FILE *f, unsigned & progress)
{
    /*
     * Read the pending reply from the device and write it out to our file.
     */

    USBProtocolMsg m(USBProtocol::Installer);
    BaseDevice base(dev);

    uint32_t header = m.header | UsbVolumeManager::FlashDeviceRead;
    if (!base.waitForReply(header, m)) {
        return false;
    }

    unsigned len = m.payloadLen();
    progress += len;

    return fwrite(m.castPayload<uint8_t>(), len, 1, f) == 1;
}
