#ifndef MANIFEST_H
#define MANIFEST_H

#include "iodevice.h"
#include "usbvolumemanager.h"
#include "bits.h"

#include <string>

class Manifest
{
public:
    Manifest(IODevice &_dev, bool rpc=false);

    static int run(int argc, char **argv, IODevice &_dev);

private:
    bool getVolumeOverview();
    bool dumpBaseSysInfo();
    bool dumpOverview();
    bool dumpVolumes();

    const char *getFirmwareVersion(USBProtocolMsg &buffer);
    UsbVolumeManager::VolumeDetailReply *getVolumeDetail(USBProtocolMsg &buffer, unsigned volBlockCode);
    const UsbVolumeManager::SysInfoReply *getBaseSysInfo(USBProtocolMsg &buffer);

    const char *getVolumeTypeString(unsigned type);

    UsbVolumeManager::VolumeOverviewReply overview;
    IODevice &dev;
    char volTypeBuffer[16];
    bool isRPC;
};

#endif // INSTALLER_H
