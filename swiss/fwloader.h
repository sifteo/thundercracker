#ifndef FW_LOADER_H
#define FW_LOADER_H

#include "usbdevice.h"

class FwLoader
{
public:
    FwLoader();

    static int run(int argc, char **argv);
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

#endif // FW_LOADER_H
