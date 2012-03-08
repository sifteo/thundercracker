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
    void run();

    reg_t reg(uint8_t r);
    void setReg(uint8_t r, reg_t val);

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
} // namespace SvmCpu

#endif // SVMCPU_H
