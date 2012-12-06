#ifndef MANIFEST_H
#define MANIFEST_H

#include "iodevice.h"
#include "usbvolumemanager.h"
#include "bits.h"
#include "basedevice.h"

#include <string>

class Manifest
{
public:
    Manifest(IODevice &_dev, bool rpc=false);

    static int run(int argc, char **argv, IODevice &_dev);

private:
    bool dumpBaseSysInfo();
    bool dumpOverview();
    bool dumpVolumes();

    const char *getVolumeTypeString(unsigned type);

    UsbVolumeManager::VolumeOverviewReply overview;
    IODevice &dev;
    BaseDevice base;
    char volTypeBuffer[16];
    bool isRPC;
};

#endif // INSTALLER_H
