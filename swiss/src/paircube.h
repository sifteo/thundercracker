#ifndef PAIRCUBE_H
#define PAIRCUBE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

class PairCube
{
public:
    PairCube(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    int pair(const char *slotStr, const char *hwidStr);
    int dumpPairingData(bool rpc);

private:
    IODevice &dev;

    static const uint64_t EMPTYSLOT = -1;   // all 0xff indicates an empty slot

    static bool getValidPairingSlot(const char *s, unsigned &pairingSlot);
    static bool getValidHWID(const char *s, uint64_t &hwid);
};

#endif // PAIRCUBE_H
