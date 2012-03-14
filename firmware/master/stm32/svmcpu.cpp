/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmcpu.h"
#include "svmruntime.h"

#include "vectors.h"

using namespace Svm;

NAKED_HANDLER ISR_SVCall()
{
    asm volatile(   "tst    lr, #0x4            \n\t"   // LR bit 2 determines whether we came from user or kernel stack
                    "ite    eq                  \n\t"   // load r0 with msp or psp based on that
                    "mrseq  r0, msp             \n\t"
                    "mrsne  r0, psp             \n\t"
                    "ldr    r1, [r0, #24]       \n\t"   // load r1 with the stacked PC
                    "ldrb   r0, [r1, #-2]       \n\t"   // extract the imm8 from the instruction into r0
                    "bl    %[target]            \n\t"   // and pass it to SvmRuntime::svc
                    "bx      lr"
                    : : [target] ""(SvmRuntime::svc));
}

namespace SvmCpu {


/***************************************************************************
 * Public Functions
 ***************************************************************************/

void init()
{
//    memset(regs, 0, sizeof(regs));
}

void run(reg_t sp, reg_t pc)
{
    for (;;) {
        asm volatile ("wfi");
    }
}

/*
 * During SVC handling, the runtime wants to operate on user space's registers,
 * which have been pushed to the stack, which we provide access to here.
 *
 * SP is special - since ARM provides a user and main SP, we operate only on
 * user SP, meaning we don't have to store it separately. However, since HW
 * automatically stacks some registers to the user SP, adjust our reporting
 * of SP's value to represent what user code will see once we have returned from
 * exception handling and HW regs have been unstacked.
 *
 * Register accessors really want to be inline, but putting that off for now, as
 * it will require a bit more code re-org to access platform specific members
 * in the common header file, etc.
 */

reg_t reg(uint8_t r)
{
    return 0;
}

void setReg(uint8_t r, reg_t val)
{
}

} // namespace SvmCpu

