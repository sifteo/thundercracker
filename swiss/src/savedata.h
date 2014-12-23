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

    static const char *SYSLFS_PACKAGE_STR;
    static const unsigned SYSLFS_VOLUME_BLOCK_CODE = 0;

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

    struct Record {
        unsigned key;
        unsigned crc;
        unsigned size;
        std::vector<uint8_t> payload;

        Record(unsigned k, unsigned c, unsigned s, const std::vector<uint8_t> &p) :
            key(k),
            crc(c),
            size(s),
            payload(p)
        {}
    };

    typedef std::map<uint8_t, std::vector<Record> > Records;

    SaveData(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    int extract(const char *pkgStr, const char *filepath, bool raw, bool rpc);
    int restore(const char *filepath);
    int normalize(const char *inpath, const char *outpath);
    int del(const char *pkgStr);

private:
    static const unsigned PAGE_SIZE = 256;
    static const unsigned BLOCK_SIZE = 128 * 1024;
    static const uint64_t MAGIC = 0x4556415374666953LLU;
    static const uint64_t NORMALIZED_MAGIC = 0x4C4D524E74666953LLU;

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

    bool getValidFileVersion(FILE *f, int &version);
    bool readHeader(int version, HeaderCommon &h, FILE *f);
    bool retrieveRecords(Records &records, const HeaderCommon &details, FILE *f);

    bool restoreRecords(unsigned vol, const Records &records);
    bool restoreItem(unsigned parentVol, const Record &record);

    static bool writeStr(const std::string &s, FILE *f);
    static bool readStr(std::string &s, FILE *f);

    static void writeNormalizedItem(std::stringstream & ss, uint8_t key, uint32_t len, const void *data);
    bool writeNormalizedRecords(Records &records, const HeaderCommon &details, FILE *f);

    IODevice &dev;
};

#endif // SAVEDATA_H
