#ifndef PAIRCUBE_H
#define PAIRCUBE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

class PairCube
{
public:
    PairCube(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool pair(const char *slotStr, const char *hwidStr);
    bool dumpPairingData();

private:
    IODevice &dev;

    static bool getValidPairingSlot(const char *s, unsigned &pairingSlot);
    static bool getValidHWID(const char *s, uint64_t &hwid);
    UsbVolumeManager::PairingSlotDetailReply *pairingSlotDetail(USBProtocolMsg &buf, unsigned pairingSlot);
};

#endif // PAIRCUBE_H
