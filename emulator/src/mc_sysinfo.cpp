/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
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

#include "sysinfo.h"
#include <stdlib.h>
#include "ostime.h"

namespace SysInfo {

    static uint8_t uidBytes[UniqueIdNumBytes];

    void init()
    {
        uint32_t *p = reinterpret_cast<uint32_t*>(&uidBytes[0]);

        for (unsigned i = 0; i < UniqueIdNumBytes / sizeof(uint32_t); ++i) {

            // ultra cheeseball. will (obviously) not be constant between
            // runs of the simulator
            *p = rand() ^ rand() ^ uint32_t(OSTime::clock() * 1e6);
            p++;
        }
    }

    const uint8_t* UniqueId = uidBytes;

    const unsigned HardwareRev = 2; // currently BOARD_TC_MASTER_REV2 in board.h
}
