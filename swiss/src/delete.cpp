#include "delete.h"
#include "usbprotocol.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

int Delete::run(int argc, char **argv, IODevice &_dev)
{
    if (argc != 2 || strcmp(argv[1], "--everything")) {
        fprintf(stderr, "currently this command only supports deleting everything.\n");
        return 1;
    }

    if (!_dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached\n");
        return 1;
    }

    Delete m(_dev);
    bool success = m.deleteEverything();

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