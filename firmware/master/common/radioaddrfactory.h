/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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
    static void convertPrimaryToAlternateChannel(RadioAddress &addr, uint8_t cubeVersion);

    static ALWAYS_INLINE void rfMinMaxChannels(uint8_t &min, uint8_t &max, uint8_t cubeVersion) {

        if (cubeVersion < CUBE_FEATURE_RF_COMPLIANT) {
            min = NONCOMPLIANT_MIN_RF_CHANNEL;
            max = NONCOMPLIANT_MAX_RF_CHANNEL;
        } else {
            min = MIN_RF_CHANNEL;
            max = MAX_RF_CHANNEL;
        }
    }

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
