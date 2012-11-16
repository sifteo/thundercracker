#include "savedata.h"
#include "basedevice.h"
#include "lfsvolume.h"
#include "util.h"
#include "bits.h"
#include "metadata.h"
#include "progressbar.h"

#include <errno.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int SaveData::run(int argc, char **argv, IODevice &_dev)
{
    bool success = false;
    SaveData saveData(_dev);

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return 1;

    if (argc >= 4 && !strcmp(argv[1], "extract")) {

        char *path = 0;
        char *volumeStr = 0;
        bool normalize = false;
        bool rpc = false;

        for (unsigned i = 2; i < argc; ++i) {
            if (!strcmp(argv[i], "--rpc")) {
                rpc = true;
            } else if (!strcmp(argv[i], "--normalize")) {
                normalize = true;
            } else if (!volumeStr) {
                volumeStr = argv[i];
            } else {
                path = argv[i];
            }
        }

        if (!path || !volumeStr) {
            fprintf(stderr, "incorrect args\n");
        } else {
            unsigned volume;
            success = Util::parseVolumeCode(volumeStr, volume) && saveData.extract(volume, path, normalize, rpc);
        }

    } else if (argc >= 3 && !strcmp(argv[1], "restore")) {

        const char *path = argv[2];
        success = saveData.restore(path);

    } else if (argc >= 4 && !strcmp(argv[1], "normalize")) {

        const char *inpath = argv[2];
        const char *outpath = argv[3];
        success = saveData.normalize(inpath, outpath);

    } else {
        fprintf(stderr, "incorrect args\n");
    }

    return success ? 0 : 1;
}

SaveData::SaveData(IODevice &_dev) :
    dev(_dev)
{}

bool SaveData::extract(unsigned volume, const char *filepath, bool normalized, bool rpc)
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

    /*
     * If normalization has been requested, write the raw file to a temp file,
     * feed it as the input to normalize(), and remove it.
     *
     * Otherwise, write the raw data to the requested file and be done.
     */

    const char *rawfilepath = normalized ? ::tmpnam(0) : filepath;

    FILE *fraw = fopen(rawfilepath, "wb");
    if (!fraw) {
        fprintf(stderr, "couldn't open %s: %s\n", filepath, strerror(errno));
        return false;
    }

    if (!writeFileHeader(fraw, volume, reply->count)) {
        return false;
    }

    if (!writeVolumes(reply, fraw, rpc)) {
        return false;
    }

    if (!normalized) {
        return true;
    }

    fclose(fraw);
    bool rv = normalize(rawfilepath, filepath);
    remove(rawfilepath);
    return rv;
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

    int fileVersion;
    if (!getValidFileVersion(fin, fileVersion)) {
        fclose(fin);
        return false;
    }

    HeaderCommon hdr;
    if (!readHeader(fileVersion, hdr, fin)) {
        fclose(fin);
        return false;
    }

    Records records;
    if (!retrieveRecords(records, hdr, fin)) {
        fclose(fin);
        return false;
    }

    for (Records::const_iterator it = records.begin(); it != records.end(); it++) {

        uint8_t key = it->first;
        std::vector<RecordData> recs = it->second;

        for (std::vector<RecordData>::const_iterator r = recs.begin(); r != recs.end(); r++) {
            printf("record. 0x%02x: %d bytes data\n", key, r->size());
        }
    }

    return false;
}

bool SaveData::normalize(const char *inpath, const char *outpath)
{
    FILE *fin = fopen(inpath, "rb");
    if (!fin) {
        fprintf(stderr, "couldn't open %s: %s\n", inpath, strerror(errno));
        return false;
    }

    FILE *fout = fopen(outpath, "wb");
    if (!fout) {
        fprintf(stderr, "couldn't open %s: %s\n", outpath, strerror(errno));
        return false;
    }

    int fileVersion;
    if (!getValidFileVersion(fin, fileVersion)) {
        fclose(fin);
        return false;
    }

    HeaderCommon hdr;
    if (!readHeader(fileVersion, hdr, fin)) {
        fclose(fin);
        return false;
    }

    Records records;
    if (!retrieveRecords(records, hdr, fin)) {
        fclose(fin);
        return false;
    }

    return writeNormalizedRecords(records, hdr, fout);
}


/********************************************************
 * Internal
 ********************************************************/


bool SaveData::getValidFileVersion(FILE *f, int &version)
{
    /*
     * Retrieve this file's version and ensure it's valid.
     */

    struct MiniHeader {
        uint64_t    magic;
        uint32_t    version;
    } minihdr;

    if (fread(&minihdr, sizeof minihdr, 1, f) != 1) {
        fprintf(stderr, "i/o error: %s\n", strerror(errno));
        return false;
    }

    if (minihdr.magic != MAGIC) {
        fprintf(stderr, "not a recognized savedata file\n");
        return false;
    }

    rewind(f);
    version = minihdr.version;

    switch (version) {
    case 0x1:
        fprintf(stderr, "savedata file version 0x1 detected,"
                "there are known errors with this version, "
                "and this data cannot be restored\n");
        return false;

    case 0x2:
        return true;

    default:
        fprintf(stderr, "unsupported savedata file version: 0x%x\n", minihdr.version);
        return false;
    }
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


bool SaveData::readHeader(int version, HeaderCommon &h, FILE *f)
{
    /*
     * Given the version, convert this file's header into HeaderCommon.
     */

    if (version == 2) {
        HeaderV2 v2;
        if (fread(&v2, sizeof v2, 1, f) != 1) {
            fprintf(stderr, "i/o error: %s\n", strerror(errno));
            return false;
        }

        h.numBlocks     = v2.numBlocks;
        h.mc_blockSize  = v2.mc_blockSize;
        h.mc_pageSize   = v2.mc_pageSize;

        memcpy(h.appUUID.bytes, v2.appUUID.bytes, sizeof(h.appUUID.bytes));
        memcpy(h.baseUniqueID, v2.baseUniqueID, sizeof(h.baseUniqueID));

        if (!readStr(h.baseFirmwareVersionStr, f) ||
            !readStr(h.packageStr, f) ||
            !readStr(h.versionStr, f))
        {
            return false;
        }

        return true;
    }

    return false;
}


bool SaveData::retrieveRecords(Records &records, const HeaderCommon &details, FILE *f)
{
    /*
     * Common implementation for all savedata file versions.
     */

    unsigned volBlockCode;
    if (!volumeCodeForPackage(details.packageStr, volBlockCode)) {
        fprintf(stderr, "cannot restore: %s in not installed\n", details.packageStr.c_str());
        return false;
    }

    for (unsigned b = 0; b < details.numBlocks; ++b) {

        LFSVolume volume(details.mc_pageSize, details.mc_blockSize);
        if (!volume.init(f)) {
            fprintf(stderr, "couldn't init volume\n");
            continue;
        }

        volume.retrieveRecords(records);
    }

    return true;
}


void SaveData::writeNormalizedItem(stringstream & ss, uint8_t key, uint32_t len, const void *data)
{
    /*
     * Each header item in a simplified savedata file looks like:
     * <uint8_t key> <uint32_t bloblen> <bloblen bytes of payload>
     */

    ss << key;
    ss.write((const char*)&len, sizeof(len));
    ss.write((const char*)data, len);
}


bool SaveData::writeNormalizedRecords(Records &records, const HeaderCommon &details, FILE *f)
{
    uint64_t magic = NORMALIZED_MAGIC;
    if (fwrite(&magic, sizeof magic, 1, f) != 1) {
        return false;
    }

    /*
     * Write headers section
     */

    stringstream hs;
    writeNormalizedItem(hs, PackageString, details.packageStr.length(), details.packageStr.c_str());
    writeNormalizedItem(hs, VersionString, details.versionStr.length(), details.versionStr.c_str());
    writeNormalizedItem(hs, UUID, sizeof(details.appUUID), &details.appUUID);
    writeNormalizedItem(hs, BaseHWID, sizeof(details.baseUniqueID), &details.baseUniqueID);
    writeNormalizedItem(hs, BaseFirmwareVersion, details.baseFirmwareVersionStr.length(), details.baseFirmwareVersionStr.c_str());

    struct NormalizedSectionHeader {
        uint32_t type;
        uint32_t length;
    } section;

    section.type = SectionHeader;
    section.length = hs.tellp();

    if (fwrite(&section, sizeof section, 1, f) != 1) {
        return false;
    }

    if (fwrite(hs.str().c_str(), section.length, 1, f) != 1) {
        return false;
    }

    /*
     * Write records section
     */

    stringstream rs;
    for (Records::const_iterator it = records.begin(); it != records.end(); it++) {

        uint8_t key = it->first;
        std::vector<RecordData> recs = it->second;

        for (std::vector<RecordData>::const_iterator r = recs.begin(); r != recs.end(); r++) {
            writeNormalizedItem(rs, key, r->size(), &(*r)[0]);
        }
    }

    section.type = SectionRecords;
    section.length = rs.tellp();

    if (fwrite(&section, sizeof section, 1, f) != 1) {
        return false;
    }

    if (fwrite(rs.str().c_str(), section.length, 1, f) != 1) {
        return false;
    }

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

    USBProtocolMsg mFWV;
    const char *fwv = base.getFirmwareVersion(mFWV);
    if (!fwv) {
        return false;
    }
    string baseFWVersion(fwv);

    Metadata metadata(dev);
    std::string packageID = metadata.getString(volBlockCode, _SYS_METADATA_PACKAGE_STR);
    std::string version = metadata.getString(volBlockCode, _SYS_METADATA_VERSION_STR);
    int uuidLen = metadata.getBytes(volBlockCode, _SYS_METADATA_UUID, hdr.appUUID.bytes, sizeof hdr.appUUID.bytes);

    if (packageID.empty() || version.empty() || uuidLen <= 0) {
        return false;
    }

    if (fwrite(&hdr, sizeof hdr, 1, f) != 1)
        return false;

    return writeStr(baseFWVersion, f) && writeStr(packageID, f) && writeStr(version, f);
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


bool SaveData::writeStr(const std::string &s, FILE *f)
{
    uint32_t length = s.length();

    if (fwrite(&length, sizeof length, 1, f) != 1)
        return false;

    if (fwrite(s.c_str(), length, 1, f) != 1)
        return false;

    return true;
}


bool SaveData::readStr(std::string &s, FILE *f)
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
