/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
