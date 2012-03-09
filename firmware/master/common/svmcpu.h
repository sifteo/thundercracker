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
    void run(reg_t sp, reg_t pc);

    reg_t reg(uint8_t r);
    void setReg(uint8_t r, reg_t val);

    void pushIrqContext();
    void popIrqContext();

} // namespace SvmCpu

#endif // SVMCPU_H
