#ifndef LOADER_H
#define LOADER_H

#include "usbdevice.h"

class Loader
{
public:
    Loader();

    bool load(const char* path, int vid, int pid);

private:

    // bootloader versions that we know how to talk to
    static const unsigned VERSION_COMPAT_MIN = 1;
    static const unsigned VERSION_COMPAT_MAX = 1;

    enum Command {
        CmdGetVersion,
        CmdWriteMemory,
        CmdSetAddrPtr,
        CmdGetAddrPtr,
        CmdJump,
        CmdAbort
    };

    bool bootloaderVersionIsCompatible();
    bool sendFirmwareFile(const char *path);

    UsbDevice dev;
};

#endif // LOADER_H
