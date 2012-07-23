/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _RADIO_ADDR_FACTORY_H
#define _RADIO_ADDR_FACTORY_H

#include <protocol.h>
#include "radio.h"
#include "prng.h"


/*
 * Algorithms for generating radio addresses
 */

class RadioAddrFactory {
public:

    static void random(RadioAddress &addr, _SYSPseudoRandomState &prng);
    static void fromHardwareID(RadioAddress &addr, uint64_t hwid);
    static void channelToggle(RadioAddress &addr);

private:
    static const uint8_t gf84[0x100];

    static ALWAYS_INLINE bool allowedByte(uint8_t b)
    {
        switch (b) {
            case 0x00:
            case 0x55:
            case 0xAA:
            case 0xFF:
                return false;
            default:
                return true;
        }
    }
};

#endif
