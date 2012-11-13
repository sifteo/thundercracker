#include "paircube.h"
#include "basedevice.h"
#include "tabularlist.h"
#include <sifteo/abi/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <iomanip>

int PairCube::run(int argc, char **argv, IODevice &_dev)
{
    PairCube pc(_dev);
    bool success = false;

    if (argc == 2 && !strcmp(argv[1], "--read")) {
        success = pc.dumpPairingData();

    } else if (argc == 3) {
        if (!strcmp(argv[1], "--read") && !strcmp(argv[2], "--rpc")) {
            success = pc.dumpPairingData(true);
        } else {
            success = pc.pair(argv[1], argv[2]);
        }

    } else {
        fprintf(stderr, "incorrect args\n");
    }

    return success ? 0 : 1;
}

PairCube::PairCube(IODevice &_dev) :
    dev(_dev)
{
}

bool PairCube::pair(const char *slotStr, const char *hwidStr)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return false;

    unsigned pairingSlot;
    if (!getValidPairingSlot(slotStr, pairingSlot))
        return false;

    uint64_t hwid;
    if (!getValidHWID(hwidStr, hwid))
        return false;

    USBProtocolMsg m;
    BaseDevice base(dev);
    return base.pairCube(m, hwid, pairingSlot);
}

bool PairCube::dumpPairingData(bool rpc)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return false;

    TabularList table;

    // Heading
    table.cell() << "SLOT";
    table.cell() << "HWID";
    table.endRow();


    BaseDevice base(dev);

    for (unsigned i = 0; i < _SYS_NUM_CUBE_SLOTS; ++i) {
        USBProtocolMsg m;
        UsbVolumeManager::PairingSlotDetailReply *reply = base.pairingSlotDetail(m, i);

        if (!reply) {
            static UsbVolumeManager::PairingSlotDetailReply zero;
            reply = &zero;
        }

        table.cell() << std::setiosflags(std::ios::hex) << std::setw(2) << std::setfill('0') << reply->pairingSlot;
        if (reply->hwid == ~0) {
            table.cell() << "(empty)";
        } else {
            table.cell() << std::setiosflags(std::ios::hex) << std::setw(16) << std::setfill('0') << reply->hwid;
        }
        table.endRow();
        
        if (rpc) {
            if (reply->hwid == ~0) {
                fprintf(stdout, "::pairing:%u:\n", reply->pairingSlot); fflush(stdout);
            } else {
                fprintf(stdout, "::pairing:%u:%llu\n", reply->pairingSlot, reply->hwid); fflush(stdout);
            }
        }
    }

    table.end();

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
    // ensure the input is of an appropriate length - hwid is 8 bytes
    const uint8_t hwidDigits = 8 * 2;
    if (strlen(s) != hwidDigits) {
        fprintf(stderr, "error, invalid HWID: incorrect length\n");
        return false;
    }

    /*
     * Verify the input is valid, and construct the 64-bit hwid.
     * This must generate the appropriate result on 64- and 32-bit machines.
     */
    for (unsigned i = 0; i < hwidDigits; i++) {

        hwid <<= 4;
        char c = s[i];

        // convert hex character to its value and add
        if (c >= '0' && c <= '9') {
            hwid += c - '0';
        }
        else if (c >= 'A' && c <= 'F') {
            hwid += c - 'A' + 10;
        }
        else if (c >= 'a' && c <= 'f') {
            hwid += c - 'a' + 10;
        }
        else {
            fprintf(stderr, "error, invalid HWID: %c is not hex\n", c);
            return false;
        }
    }

    return true;
}
