#include "delete.h"
#include "basedevice.h"
#include "util.h"
#include "usbprotocol.h"
#include "swisserror.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int Delete::run(int argc, char **argv, IODevice &_dev)
{
    if (argc != 2) {
        fprintf(stderr, "incorrect args\n");
        return EINVAL;
    }

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return ENODEV;
    }

    Delete m(_dev);
    unsigned volCode;

    if (!strcmp(argv[1], "--all")) {
        return m.deleteEverything();
    }

    if (!strcmp(argv[1], "--reformat")) {
        return m.deleteReformat();
    }

    if (!strcmp(argv[1], "--sys")) {
        return m.deleteSysLFS();
    }

    if (m.getVolumeCode(argv[1], volCode)) {
        return m.deleteVolume(volCode);
    }

    fprintf(stderr, "incorrect args\n");
    return EINVAL;
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

int Delete::deleteEverything()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteEverything;

    if (!BaseDevice(dev).writeAndWaitForReply(m)) {
        return EIO;
    }

    return EOK;
}

int Delete::deleteReformat()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteReformat;

    fprintf(stderr, "This will take a few minutes...\n");

    if (!BaseDevice(dev).writeAndWaitForReply(m)) {
        return EIO;
    }

    return EOK;
}

int Delete::deleteSysLFS()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteSysLFS;

    if (!BaseDevice(dev).writeAndWaitForReply(m)) {
        return EIO;
    }

    return EOK;
}

int Delete::deleteVolume(unsigned code)
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteVolume;
    m.append((uint8_t*) &code, sizeof code);

    if (!BaseDevice(dev).writeAndWaitForReply(m)) {
        return EIO;
    }

    return EOK;
}
