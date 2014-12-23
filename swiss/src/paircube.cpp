/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "paircube.h"
#include "basedevice.h"
#include "tabularlist.h"
#include "macros.h"
#include "swisserror.h"

#include <sifteo/abi/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <iomanip>

int PairCube::run(int argc, char **argv, IODevice &_dev)
{
    PairCube pc(_dev);

    if (argc >= 2) {

        if (!strcmp(argv[1], "--read")) {
            bool rpc = false;
            if (argc >= 3 && strcmp(argv[2], "--rpc") == 0) {
                rpc = true;
            }

            return pc.dumpPairingData(rpc);
        }
    }

    if (argc == 3) {
        return pc.pair(argv[1], argv[2]);
    }

    fprintf(stderr, "incorrect args\n");
    return EINVAL;
}

PairCube::PairCube(IODevice &_dev) :
    dev(_dev)
{
}

int PairCube::pair(const char *slotStr, const char *hwidStr)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return ENODEV;
    }

    unsigned pairingSlot;
    if (!getValidPairingSlot(slotStr, pairingSlot)) {
        return EINVAL;
    }

    uint64_t hwid;
    if (!getValidHWID(hwidStr, hwid)) {
        return EINVAL;
    }

    USBProtocolMsg m;
    BaseDevice base(dev);
    if (!base.pairCube(m, hwid, pairingSlot)) {
        return EIO;
    }

    return EOK;
}

int PairCube::dumpPairingData(bool rpc)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return ENODEV;
    }

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
        if (reply->hwid == EMPTYSLOT) {
            table.cell() << "(empty)";
        } else {
            table.cell() << std::setiosflags(std::ios::hex) << std::setw(16) << std::setfill('0') << reply->hwid;
        }
        table.endRow();
        
        if (rpc) {
            if (reply->hwid == EMPTYSLOT) {
                fprintf(stdout, "::pairing:%u:\n", reply->pairingSlot); fflush(stdout);
            } else {
                fprintf(stdout, "::pairing:%u:%"PRIu64"\n", reply->pairingSlot, reply->hwid); fflush(stdout);
            }
        }
    }

    table.end();

    return EOK;
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
