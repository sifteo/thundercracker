#ifndef PAIRCUBE_H
#define PAIRCUBE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

class PairCube
{
public:
    PairCube(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool pair(uint8_t pairingSlot, uint64_t hwid);

private:
    IODevice &dev;

    static bool getValidPairingSlot(const char *s, unsigned &pairingSlot);
    static bool getValidHWID(const char *s, uint64_t &hwid);
};

#endif // PAIRCUBE_H
