/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2011 Sifteo, Inc.
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

#ifndef _CUBE_RNG_H
#define _CUBE_RNG_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "vtime.h"
#include "cube_cpu_reg.h"

namespace Cube {


class RNG {
 public:

    void init() {
        timer = 0;
    }

    uint8_t controlRead(VirtualTime &vtime, CPU::em8051 &cpu) {
        return (cpu.mSFR[REG_RNGCTL] & (RNGCTL_PWRUP | RNGCTL_CORRECTOR)) |
               (timer <= vtime.clocks ? RNGCTL_READY : 0);
    }
    
    uint8_t dataRead(VirtualTime &vtime, CPU::em8051 &cpu) {
        if ((cpu.mSFR[REG_RNGCTL] & RNGCTL_PWRUP) && timer <= vtime.clocks) {
            setTimer(vtime);
            return rand();
        }
        
        CPU::except(&cpu, CPU::EXCEPTION_RNG);
        return 0;
    }
    
    void controlWrite(VirtualTime &vtime, CPU::em8051 &cpu) {
        if (cpu.mSFR[REG_RNGCTL] & RNGCTL_PWRUP)
            setTimer(vtime);
    }
    
 private:
    uint64_t timer;        
    
    void setTimer(VirtualTime &vtime) {
        timer = vtime.clocks + vtime.usec(400);
    }
    
    static const uint8_t RNGCTL_PWRUP         = 0x80;
    static const uint8_t RNGCTL_CORRECTOR     = 0x40;
    static const uint8_t RNGCTL_READY         = 0x20;
};
 

};  // namespace Cube

#endif
