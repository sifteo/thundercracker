#ifndef FW_LOADER_H
#define FW_LOADER_H

#include "iodevice.h"
#include <stdio.h>

class FwLoader
{
public:
    FwLoader(IODevice &_dev, bool rpc=false);

    static int run(int argc, char **argv, IODevice &dev);
    int requestBootloaderUpdate(unsigned int pid);
    int load(const char* path, unsigned int pid);

private:

    // bootloader versions that we know how to talk to
    static const unsigned VERSION_COMPAT_MIN = 1;
    static const unsigned VERSION_COMPAT_MAX = 1;

    // the hardware version to assume for bootloaders that don't
    // report one explicitly. Only rev 2 masters have been released
    // in the wild with this bootloader.
    static const unsigned DEFAULT_HW_VERSION = 2;

    struct Header {
        uint32_t key;
        uint32_t size;
    };

    bool bootloaderVersionIsCompatible(unsigned &swVersion, unsigned &hwVersion);
    void resetBootloader();
    bool checkFileDetails(FILE *f, uint32_t &plainsz, uint32_t &crc, long fileEnd);
    bool sendFirmwareFile(FILE *f, uint32_t crc, uint32_t plaintextSize, unsigned fileSize);
    bool readFirmwareBinaryHeader(FILE *f, uint32_t &hwVersion, uint32_t &fwSize);

    int loadSingle(FILE *f);
    int loadContainer(FILE *f);

    bool installFile(FILE *f, unsigned fileSize);

    IODevice &dev;
    bool isRPC;
};

#endif // FW_LOADER_H
