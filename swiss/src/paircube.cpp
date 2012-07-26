#include "paircube.h"
#include <stdio.h>
#include <stdlib.h>
#include <sifteo/abi/types.h>

int PairCube::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 3) {
        fprintf(stderr, "pair: incorrect args\n");
        return 1;
    }

    unsigned pairingID;
    if (!getValidPairingSlot(argv[1], pairingID))
        return 1;

    uint64_t hwid;
    if (!getValidHWID(argv[2], hwid))
        return 1;

    PairCube pc(_dev);
    bool success = pc.pair(pairingID, hwid);

    _dev.close();
    _dev.processEvents();

    return success ? 0 : 1;
}

PairCube::PairCube(IODevice &_dev) :
    dev(_dev)
{
}

bool PairCube::pair(uint8_t pairingSlot, uint64_t hwid)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= UsbVolumeManager::PairCube;

    UsbVolumeManager::PairCubeRequest *req = m.zeroCopyAppend<UsbVolumeManager::PairCubeRequest>();
    req->hwid = hwid;
    req->pairingSlot = pairingSlot;

    dev.writePacket(m.bytes, m.len);
    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    // check response
    m.len = dev.readPacket(m.bytes, m.MAX_LEN);
    if ((m.header & 0xff) != UsbVolumeManager::PairCube) {
        fprintf(stderr, "unexpected response\n");
        return false;
    }

    return true;
}

bool PairCube::getValidPairingSlot(const char *s, unsigned &pairingSlot)
{
    long int id = strtol(s, 0, 0);
    if (id < 0 || id >= _SYS_NUM_CUBE_SLOTS) {
        fprintf(stderr, "error: pairing slot out of valid range (0 - %d)\n", _SYS_NUM_CUBE_SLOTS - 1);
        return false;
    }

    pairingSlot = id;
    return true;
}

bool PairCube::getValidHWID(const char *s, uint64_t &hwid)
{
    int rv = sscanf(s, "%llx", &hwid);
    if (rv != 1 || hwid == ~0) {
        fprintf(stderr, "error: invalid HWID: 0x%llx\n", hwid);
        return false;
    }

    return true;
}
