#ifndef SAVEDATA_H
#define SAVEDATA_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

class SaveData
{
public:
    SaveData(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool extract(unsigned volume, const char *filepath);
    bool restore(const char *filepath);

private:
    static const unsigned PAGE_SIZE = 256;
    static const unsigned BLOCK_SIZE = 64 * 1024;

    UsbVolumeManager::LFSDetailReply *getLFSDetail(USBProtocolMsg &buffer, unsigned volBlockCode);
    bool writeFileHeader(FILE *f, unsigned volBlockCode, unsigned numVolumes);
    bool extractLFSVolume(unsigned address, unsigned len, FILE *f);

    bool writeVolumes(UsbVolumeManager::LFSDetailReply *reply, FILE *f);
    bool sendRequest(unsigned baseAddr, unsigned &progress);
    bool writeReply(FILE *f, unsigned &progress);

    IODevice &dev;
};

#endif // SAVEDATA_H
