#ifndef MANIFEST_H
#define MANIFEST_H

#include "iodevice.h"
#include "usbvolumemanager.h"
#include "bits.h"

#include <string>

class Manifest
{
public:
    Manifest(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

private:
    bool getVolumeOverview();
    bool dumpVolumes();

    bool getMetadata(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key);
    const char *getMetadataString(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key);
    UsbVolumeManager::VolumeDetailReply *getVolumeDetail(USBProtocolMsg &buffer, unsigned volBlockCode);

    const char *getVolumeTypeString(unsigned type);

    UsbVolumeManager::VolumeOverviewReply overview;
    IODevice &dev;
    char volTypeBuffer[16];
};

#endif // INSTALLER_H
