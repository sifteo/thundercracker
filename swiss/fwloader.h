#ifndef FW_LOADER_H
#define FW_LOADER_H

#include "iodevice.h"

class FwLoader
{
public:
    FwLoader(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &dev);
    bool load(const char* path, int vid, int pid);

private:

    // bootloader versions that we know how to talk to
    static const unsigned VERSION_COMPAT_MIN = 1;
    static const unsigned VERSION_COMPAT_MAX = 1;

    bool bootloaderVersionIsCompatible();
    void resetWritePointer();
    bool sendFirmwareFile(const char *path);

    IODevice &dev;
};

#endif // FW_LOADER_H
