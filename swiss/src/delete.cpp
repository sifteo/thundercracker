#include "delete.h"
#include "usbprotocol.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int Delete::run(int argc, char **argv, IODevice &_dev)
{
    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached\n");
        return 1;
    }

    Delete m(_dev);
    bool success = false;
    unsigned volCode;

    if (argc == 2 && !strcmp(argv[1], "--all")) {
        success = m.deleteEverything();

    } else if (argc == 2 && !strcmp(argv[1], "--sys")) {
        success = m.deleteSysLFS();

    } else if (argc == 2 && parseVolumeCode(argv[1], volCode)) {
        success = m.deleteVolume(volCode);

    } else {
        fprintf(stderr, "incorrect args\n");
    }

    _dev.close();
    _dev.processEvents();

    return success ? 0 : 1;
}

Delete::Delete(IODevice &_dev) :
    dev(_dev)
{}

bool Delete::deleteEverything()
{
    USBProtocolMsg m(USBProtocol::Installer);

    m.header |= UsbVolumeManager::DeleteEverything;
    dev.writePacket(m.bytes, m.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::DeleteEverything) {
        fprintf(stderr, "unexpected response\n");
        return false;
    }

    return true;
}

bool Delete::deleteSysLFS()
{
    USBProtocolMsg m(USBProtocol::Installer);

    m.header |= UsbVolumeManager::DeleteSysLFS;
    dev.writePacket(m.bytes, m.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::DeleteSysLFS) {
        fprintf(stderr, "unexpected response\n");
        return false;
    }

    return true;
}

bool Delete::deleteVolume(unsigned code)
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::DeleteVolume;
    m.append((uint8_t*) &code, sizeof code);

    dev.writePacket(m.bytes, m.len);

    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::DeleteVolume) {
        fprintf(stderr, "unexpected response\n");
        return false;
    }

    return true;
}

bool Delete::parseVolumeCode(const char *str, unsigned &code)
{
    /*
     * Volume codes are 8-bit hexadecimal strings.
     * Returns true and fills in 'code' if valid.
     */

    if (!*str)
        return false;

    char *end;
    code = strtol(str, &end, 16);
    return !*end && code < 0x100;
}
