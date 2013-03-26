#include "delete.h"
#include "basedevice.h"
#include "util.h"
#include "usbprotocol.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int Delete::run(int argc, char **argv, IODevice &_dev)
{
    if (argc != 2) {
        fprintf(stderr, "incorrect args\n");
        return 1;
    }

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return 1;

    Delete m(_dev);
    bool success = false;
    unsigned volCode;

    if (!strcmp(argv[1], "--all")) {
        success = m.deleteEverything();

    } else if (!strcmp(argv[1], "--reformat")) {
        success = m.deleteReformat();

    } else if (!strcmp(argv[1], "--sys")) {
        success = m.deleteSysLFS();

    } else if (m.getVolumeCode(argv[1], volCode)) {
        success = m.deleteVolume(volCode);

    } else {
        fprintf(stderr, "incorrect args\n");
    }

    return success ? 0 : 1;
}

Delete::Delete(IODevice &_dev) :
    dev(_dev)
{}

bool Delete::getVolumeCode(const char *s, unsigned &volumeCode)
{
    // try to parse it as a hex value.
    // historically, this was the only mechanism we offered for deletion.
    if (Util::parseVolumeCode(s, volumeCode)) {
        return true;
    }

    // try to look it up as a package name
    if (BaseDevice(dev).volumeCodeForPackage(s, volumeCode)) {
        return true;
    }

    return false;
}

bool Delete::deleteEverything()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteEverything;

    return BaseDevice(dev).writeAndWaitForReply(m);
}

bool Delete::deleteReformat()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteReformat;

    fprintf(stderr, "This will take a few minutes...\n");

    return BaseDevice(dev).writeAndWaitForReply(m);
}

bool Delete::deleteSysLFS()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteSysLFS;

    return BaseDevice(dev).writeAndWaitForReply(m);
}

bool Delete::deleteVolume(unsigned code)
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteVolume;
    m.append((uint8_t*) &code, sizeof code);

    return BaseDevice(dev).writeAndWaitForReply(m);
}
