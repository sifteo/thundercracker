/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implements the nRF24LE1's Multiply Divide Unit (MDU), a tiny math coprocessor.
 *
 * XXX: Doesn't support reporting errors/overflows via ARCON yet.
 */
 
#ifndef _CUBE_MDU_H
#define _CUBE_MDU_H

#include <stdint.h>

#include "vtime.h"
#include "cube_cpu.h"

namespace Cube {


class MDU {
public:

    void init()
    {
        busy_timer = 0;
        write_sequence = 0xFFFFFFFF;
    }
    
    ALWAYS_INLINE void write(const VirtualTime &vtime, CPU::em8051 &cpu, int reg)
    {
        if (reg == 0) {
            // MD0 is always the first write in a sequence
            write_sequence = 0;
         
        } else {
            // Keep track of what order MD1 through MD5 were written in
            write_sequence = (write_sequence << 4) | reg;
        
            if (reg >= 5)
                operation(vtime, cpu);
        }
    }
    
    ALWAYS_INLINE int read(const VirtualTime &vtime, CPU::em8051 &cpu, int reg)
    {
        /*
         * We store results during operation(), but if the firmware tries
         * to read them too early, fire an exception.
         *
         * We don't perform this check if we're in static binary translation,
         * since the entire MDU operation may have occurred within the same basic block.
         */
         
        if (vtime.clocks < busy_timer && !cpu.sbt)
            CPU::except(&cpu, CPU::EXCEPTION_MDU);
        
        return cpu.mSFR[reg + REG_MD0];
    }

private:
    uint64_t busy_timer;
    uint32_t write_sequence;
            
    void operation(const VirtualTime &vtime, CPU::em8051 &cpu)
    {
        // Still busy
        if (vtime.clocks < busy_timer)
            CPU::except(&cpu, CPU::EXCEPTION_MDU);
    
        switch (write_sequence) {
        
        // 32/16 division
        case 0x54321: {
            uint32_t a = ((uint32_t)cpu.mSFR[REG_MD3] << 24) |
                         ((uint32_t)cpu.mSFR[REG_MD2] << 16) |
                         ((uint32_t)cpu.mSFR[REG_MD1] << 8 ) |
                         ((uint32_t)cpu.mSFR[REG_MD0] << 0 );
            uint16_t b = ((uint16_t)cpu.mSFR[REG_MD5] << 8 ) |
                         ((uint16_t)cpu.mSFR[REG_MD4] << 0 );
            
            uint32_t q = a/b;
            uint16_t r = a%b;
            busy_timer = vtime.clocks + 17;
            
            cpu.mSFR[REG_MD0] = q >> 0;
            cpu.mSFR[REG_MD1] = q >> 8;
            cpu.mSFR[REG_MD2] = q >> 16;
            cpu.mSFR[REG_MD3] = q >> 24;
            cpu.mSFR[REG_MD4] = r >> 0;
            cpu.mSFR[REG_MD5] = r >> 8;
            break;
        }
        
        // 16/16 division
        case 0x541: {
            uint16_t a = ((uint16_t)cpu.mSFR[REG_MD1] << 8 ) |
                         ((uint16_t)cpu.mSFR[REG_MD0] << 0 );
            uint16_t b = ((uint16_t)cpu.mSFR[REG_MD5] << 8 ) |
                         ((uint16_t)cpu.mSFR[REG_MD4] << 0 );

            uint16_t q = a/b;
            uint16_t r = a%b;
            busy_timer = vtime.clocks + 9;
            
            cpu.mSFR[REG_MD0] = q >> 0;
            cpu.mSFR[REG_MD1] = q >> 8;
            cpu.mSFR[REG_MD4] = r >> 0;
            cpu.mSFR[REG_MD5] = r >> 8;
            break;
        }
        
        // 16x16 multiplication
        case 0x514: {
            uint32_t a = ((uint16_t)cpu.mSFR[REG_MD1] << 8 ) |
                         ((uint16_t)cpu.mSFR[REG_MD0] << 0 );
            uint32_t b = ((uint16_t)cpu.mSFR[REG_MD5] << 8 ) |
                         ((uint16_t)cpu.mSFR[REG_MD4] << 0 );
            
            uint32_t c = a*b;
            busy_timer = vtime.clocks + 11;
            
            cpu.mSFR[REG_MD0] = c >> 0;
            cpu.mSFR[REG_MD1] = c >> 8;
            cpu.mSFR[REG_MD2] = c >> 16;
            cpu.mSFR[REG_MD3] = c >> 24;                  
            break;
        }
    
        // Undocumented: Normalize/shift without rewriting all operands
        case 0x6:
            // Fall through...
        
        // Normalize/shift
        case 0x6321: {
            uint32_t n = ((uint32_t)cpu.mSFR[REG_MD3] << 24) |
                         ((uint32_t)cpu.mSFR[REG_MD2] << 16) |
                         ((uint32_t)cpu.mSFR[REG_MD1] << 8 ) |
                         ((uint32_t)cpu.mSFR[REG_MD0] << 0 );
            uint8_t con = cpu.mSFR[REG_ARCON];

            if ((con & 0x1F) == 0) {
                // Normalize
                
                if (n)
                    while ((n & 0x80000000) == 0) {
                        n <<= 1;
                        con++;
                    }
                    
                busy_timer = vtime.clocks + 4 + (con & 0x1F)/2;

            } else {            
                
                if (con & 0x20) {
                    // Right shift
                    n >>= con & 0x1F;
                } else {
                    // Left shift
                    n <<= con & 0x1F;
                }
                
                busy_timer = vtime.clocks + 3 + (con & 0x1F)/2;
            }
            
            cpu.mSFR[REG_MD0] = n >> 0;
            cpu.mSFR[REG_MD1] = n >> 8;
            cpu.mSFR[REG_MD2] = n >> 16;
            cpu.mSFR[REG_MD3] = n >> 24;        
            cpu.mSFR[REG_ARCON] = con;
            break;
        }
    
        // Illegal write order
        default:             
            CPU::except(&cpu, CPU::EXCEPTION_MDU);
            break;
        }
    }
};
 

};  // namespace Cube

#endif
