/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMCPU_H
#define SVMCPU_H

#include "svm.h"

namespace SvmCpu {

    using namespace Svm;

    void init();
#ifdef SIFTEO_SIMULATOR
    void run(reg_t sp, reg_t pc) __attribute__ ((noreturn));
#else
    void run(reg_t sp, reg_t pc) __attribute__ ((naked));
#endif

    reg_t reg(uint8_t r);
    void setReg(uint8_t r, reg_t val);

    /*****************************************************************
     * These structs are only used internally to SvmCpu, but including
     * them here so platform specific implementations can share them.
     * We can hide them in another internal namespace if we need to.
     ****************************************************************/

    // registers that get saved to the stack automatically by hardware
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

    // registers that we want to operate on during exception handling
    // which do not get saved by hardware
    // TODO: may be able to get away without saving all these
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

} // namespace SvmCpu

#endif // SVMCPU_H
