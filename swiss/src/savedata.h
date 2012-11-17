#ifndef SAVEDATA_H
#define SAVEDATA_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>
#include <map>
#include <vector>

class SaveData
{
public:
    static const unsigned VERSION = 2;

    enum NormalizedSectionTypes {
        SectionHeader,
        SectionRecords
    };

    enum NormalizedHeaderTypes {
        PackageString,
        VersionString,
        UUID,
        BaseHWID,
        BaseFirmwareVersion
    };

    /*
     * We capture all records for a given key, and make sure they're
     * ordered chronologically.
     */
    typedef std::vector<uint8_t> RecordData;
    typedef std::map<uint8_t, std::vector<RecordData> > Records;

    SaveData(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool extract(unsigned volume, const char *filepath, bool raw, bool rpc);
    bool restore(const char *filepath);
    bool normalize(const char *inpath, const char *outpath);

private:
    static const unsigned PAGE_SIZE = 256;
    static const unsigned BLOCK_SIZE = 128 * 1024;
    static const uint64_t MAGIC = 0x4556415374666953LLU;
    static const uint64_t NORMALIZED_MAGIC = 0x4C4D524E74666953LLU;

    static const unsigned SYSLFS_VOLUME_BLOCK_CODE = 0;
    static const char *SYSLFS_PACKAGE_STR;

    // V1 files had a fatal error in one of their size calculations.
    // we can't get anything useful from them, so treat them as unsupported.

    struct HeaderV2 {
        uint64_t    magic;
        uint32_t    version;
        uint32_t    numBlocks;
        uint32_t    mc_pageSize;
        uint32_t    mc_blockSize;
        _SYSUUID    appUUID;
        uint8_t     baseUniqueID[SysInfo::UniqueIdNumBytes];
        uint32_t    baseHwRevision;
        // variable size
        // uint32_t len, bytes of base firmware version
        // uint32_t len, bytes of game package string
        // uint32_t len, bytes of game version string
    };

    // common elements from a header required for our actual operations
    struct HeaderCommon {
        uint32_t    numBlocks;
        uint32_t    mc_pageSize;
        uint32_t    mc_blockSize;
        _SYSUUID    appUUID;
        uint8_t     baseUniqueID[SysInfo::UniqueIdNumBytes];
        std::string baseFirmwareVersionStr;
        std::string packageStr;
        std::string versionStr;
    };

    UsbVolumeManager::LFSDetailReply *getLFSDetail(USBProtocolMsg &buffer, unsigned volBlockCode);
    bool writeFileHeader(FILE *f, unsigned volBlockCode, unsigned numVolumes);

    bool writeVolumes(UsbVolumeManager::LFSDetailReply *reply, FILE *f, bool rpc=false);
    bool sendRequest(unsigned baseAddr, unsigned &progress);
    bool writeReply(FILE *f, unsigned &progress);

    bool volumeCodeForPackage(const std::string & pkg, unsigned &volumeCode);

    bool getValidFileVersion(FILE *f, int &version);
    bool readHeader(int version, HeaderCommon &h, FILE *f);
    bool retrieveRecords(Records &records, const HeaderCommon &details, FILE *f);

    static bool writeStr(const std::string &s, FILE *f);
    static bool readStr(std::string &s, FILE *f);

    static void writeNormalizedItem(std::stringstream & ss, uint8_t key, uint32_t len, const void *data);
    bool writeNormalizedRecords(Records &records, const HeaderCommon &details, FILE *f);

    IODevice &dev;
};

#endif // SAVEDATA_H
