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

#ifndef SVMCPU_H
#define SVMCPU_H

#include "svm.h"
#include "macros.h"

/*
 * Instruction-level tracing, available in emulation only.
 */
#ifdef SIFTEO_SIMULATOR
#   include "system.h"
#   include "system_mc.h"
#   define TRACING_ONLY(x) \
        do { \
            if (SystemMC::getSystem()->opt_svmTrace) { \
                x ; \
            } \
        } while (0)
#else
#   define TRACING_ONLY(x)
#endif

/*
 * Implementation-specific function attributes for run()
 */
#ifdef SIFTEO_SIMULATOR
#   define SVM_RUN_ATTRS __attribute__ ((noreturn))
#else
#   define SVM_RUN_ATTRS __attribute__ ((naked))
#endif


namespace SvmCpu {
    using namespace Svm;

    void run(reg_t sp, reg_t pc) SVM_RUN_ATTRS;

    // Registers that get saved to the stack automatically by hardware
    struct HwContext {
        reg_t r0;
        reg_t r1;
        reg_t r2;
        reg_t r3;
        reg_t r12;
        reg_t lr;
        reg_t returnAddr;
        reg_t xpsr;
    };

    // Registers that we want to operate on during exception handling
    // which do not get saved by hardware
    struct IrqContext {
        reg_t r4;
        reg_t r5;
        reg_t r6;
        reg_t r7;
        reg_t r8;
        reg_t r9;
        reg_t r10;
        reg_t r11;
    };
    
    // All user-mode registers
    struct UserRegs {
        IrqContext irq;
        HwContext hw;
        reg_t sp;
    };

    // Global storage location for user-mode registers
    extern UserRegs userRegs;

    /*
     * Inlined register accessors.
     *
     * These fold into very simple code for constant values of 'r',
     * or generate inlined switch statements for cases when 'r' is
     * not known at compile-time.
     */

    ALWAYS_INLINE reg_t reg(unsigned r)
    {
        switch (r) {
        case 0:         return userRegs.hw.r0;
        case 1:         return userRegs.hw.r1;
        case 2:         return userRegs.hw.r2;
        case 3:         return userRegs.hw.r3;
        case 4:         return userRegs.irq.r4;
        case 5:         return userRegs.irq.r5;
        case 6:         return userRegs.irq.r6;
        case 7:         return userRegs.irq.r7;
        case 8:         return userRegs.irq.r8;
        case 9:         return userRegs.irq.r9;
        case 10:        return userRegs.irq.r10;
        case 11:        return userRegs.irq.r11;
        case 12:        return userRegs.hw.r12;
        case REG_SP:    return userRegs.sp + sizeof(HwContext);
        case REG_PC:    return userRegs.hw.returnAddr;
        case REG_CPSR:  return userRegs.hw.xpsr;
        default:        return 0;
        }
    }

    ALWAYS_INLINE void setReg(unsigned r, reg_t val)
    {
        switch (r) {
        case 0:         userRegs.hw.r0 = val; break;
        case 1:         userRegs.hw.r1 = val; break;
        case 2:         userRegs.hw.r2 = val; break;
        case 3:         userRegs.hw.r3 = val; break;
        case 4:         userRegs.irq.r4 = val; break;
        case 5:         userRegs.irq.r5 = val; break;
        case 6:         userRegs.irq.r6 = val; break;
        case 7:         userRegs.irq.r7 = val; break;
        case 8:         userRegs.irq.r8 = val; break;
        case 9:         userRegs.irq.r9 = val; break;
        case 10:        userRegs.irq.r10 = val; break;
        case 11:        userRegs.irq.r11 = val; break;
        case 12:        userRegs.hw.r12 = val; break;
        case REG_SP:    userRegs.sp = val - sizeof(HwContext); break;
        case REG_PC:    userRegs.hw.returnAddr = val; break;
        case REG_CPSR:  userRegs.hw.xpsr = val; break;
        }
    }

    // For cases where the arg is not constant, but it's in the range [0, 7]
    ALWAYS_INLINE reg_t reg07(unsigned r)
    {
        switch (r) {
        case 0:         return userRegs.hw.r0;
        case 1:         return userRegs.hw.r1;
        case 2:         return userRegs.hw.r2;
        case 3:         return userRegs.hw.r3;
        case 4:         return userRegs.irq.r4;
        case 5:         return userRegs.irq.r5;
        case 6:         return userRegs.irq.r6;
        case 7:         return userRegs.irq.r7;
        default:        return 0;
        }
    }

    // For cases where the arg is not constant, but it's in the range [0, 7]
    ALWAYS_INLINE void setReg07(unsigned r, reg_t val)
    {
        switch (r) {
        case 0:         userRegs.hw.r0 = val; break;
        case 1:         userRegs.hw.r1 = val; break;
        case 2:         userRegs.hw.r2 = val; break;
        case 3:         userRegs.hw.r3 = val; break;
        case 4:         userRegs.irq.r4 = val; break;
        case 5:         userRegs.irq.r5 = val; break;
        case 6:         userRegs.irq.r6 = val; break;
        case 7:         userRegs.irq.r7 = val; break;
    }
}


} // namespace SvmCpu

#endif // SVMCPU_H
