#ifndef SAVEDATA_H
#define SAVEDATA_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

class SaveData
{
public:
    static const unsigned VERSION = 2;

    SaveData(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool extract(unsigned volume, const char *filepath, bool rpc=false);
    bool restore(const char *filepath);

private:
    static const unsigned PAGE_SIZE = 256;
    static const unsigned BLOCK_SIZE = 64 * 1024;
    static const uint64_t MAGIC = 0x4556415374666953LLU;

    struct HeaderV1 {
        uint64_t    magic;
        uint32_t    version;
        uint32_t    numBlocks;
        uint32_t    mc_pageSize;
        uint32_t    mc_blockSize;
        _SYSUUID    appUUID;
        // variable size
        // uint32_t len, bytes of game package string
        // uint32_t len, bytes of game version string
    };

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
        // uint32_t len, bytes of game package string
        // uint32_t len, bytes of game version string
    };

    UsbVolumeManager::LFSDetailReply *getLFSDetail(USBProtocolMsg &buffer, unsigned volBlockCode);
    bool writeFileHeader(FILE *f, unsigned volBlockCode, unsigned numVolumes);
    bool extractLFSVolume(unsigned address, unsigned len, FILE *f);

    bool writeVolumes(UsbVolumeManager::LFSDetailReply *reply, FILE *f, bool rpc=false);
    bool sendRequest(unsigned baseAddr, unsigned &progress);
    bool writeReply(FILE *f, unsigned &progress);

    IODevice &dev;
};

#endif // SAVEDATA_H
