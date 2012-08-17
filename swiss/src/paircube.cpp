#include "paircube.h"
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

bool PairCube::dumpPairingData(bool rpc)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return false;

    TabularList table;

    // Heading
    table.cell() << "SLOT";
    table.cell() << "HWID";
    table.endRow();

    for (unsigned i = 0; i < _SYS_NUM_CUBE_SLOTS; ++i) {
        USBProtocolMsg m;
        UsbVolumeManager::PairingSlotDetailReply *reply = pairingSlotDetail(m, i);

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
    int rv = sscanf(s, "%llx", &hwid);
    if (rv != 1 || hwid == ~0) {
        fprintf(stderr, "error: invalid HWID: 0x%llx\n", hwid);
        return false;
    }

    return true;
}

UsbVolumeManager::PairingSlotDetailReply *PairCube::pairingSlotDetail(USBProtocolMsg &buf, unsigned pairingSlot)
{
    buf.init(USBProtocol::Installer);
    buf.header |= UsbVolumeManager::PairingSlotDetail;
    buf.append((uint8_t*) &pairingSlot, sizeof pairingSlot);

    dev.writePacket(buf.bytes, buf.len);
    while (dev.numPendingINPackets() == 0) {
        dev.processEvents();
    }

    // check response
    buf.len = dev.readPacket(buf.bytes, buf.MAX_LEN);
    if ((buf.header & 0xff) != UsbVolumeManager::PairingSlotDetail) {
        fprintf(stderr, "unexpected response\n");
        return 0;
    }

    if (buf.payloadLen() >= sizeof(UsbVolumeManager::PairingSlotDetailReply))
        return buf.castPayload<UsbVolumeManager::PairingSlotDetailReply>();

    return 0;
}
