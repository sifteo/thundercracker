/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmcpu.h"
#include "svmruntime.h"
#include "ui_panic.h"

#include "vectors.h"

using namespace Svm;

namespace SvmCpu {

UserRegs userRegs;

/*
 * Run an application.
 *
 * Set the user stack pointer to the provided value,
 * and write the CONTROL special register to both switch to user mode,
 * and start using the user mode stack.
 *
 * Finally, branch into user code.
 *
 * Note: when branching with bx, target's bit[0] must be set (indicating thumb mode)
 * or we get a usage fault since cortex-m3 cannot exchange to arm mode.
 */
void run(reg_t sp, reg_t pc)
{
    asm volatile(
        "msr    psp, %[sp_arg]          \n\t"
        "mov    r10, %[target]          \n\t"
        "mov    r0,  #0                 \n\t"
        "mov    r1,  #0                 \n\t"
        "mov    r2,  #0                 \n\t"
        "mov    r3,  #0                 \n\t"
        "mov    r4,  #0                 \n\t"
        "mov    r5,  #0                 \n\t"
        "mov    r6,  #0                 \n\t"
        "mov    r7,  #0                 \n\t"
        "mov    r8,  #0                 \n\t"
        "mov    r9,  #0                 \n\t"
        "mov    r11, #0                 \n\t"
        "mov    r12, #0x3               \n\t"
        "msr    control, r12            \n\t"
        "mov    r12, #0                 \n\t"
        "isb                            \n\t"
        "bx     r10"
        :
        : [sp_arg] "r"(sp), [target] "r"(pc | 0x1)
    );

    // Cannot be reached unless we jumped into bad code that the validator failed to catch!
    UIPanic::haltForever();
}

} // namespace SvmCpu

/*
 * SVC handler.
 * Copy cortex-m3 stacked registers (HwContext) to trusted memory,
 * and copy regs 4-11 to trusted memory for manipulation by the runtime.
 *
 * After runtime handling, the desired user stack pointer may have been modified,
 * so copy the hardware stacked regs to this location, and update the user sp
 * before exiting such that HW unstacking finds them at the correct location.
 */
NAKED_HANDLER ISR_SVCall()
{
    asm volatile(
        "tst    lr, #0x4            \n\t"   // LR bit 2 determines if regs were stacked to user or main stack
        "ite    eq                  \n\t"   // load r0 with msp or psp based on that
        "mrseq  r0, msp             \n\t"
        "mrsne  r0, psp             \n\t"

        "ldr    r2, =%[usrirq]      \n\t"   // load pointer to userRegs.irq into r2
        "stm    r2, { r4-r11 }      \n\t"   // store copy of user regs

        "ldr    r1, =%[savedSp]     \n\t"   // copy psp (r0) to user sp (r1)
        "str    r0, [r1]            \n\t"

        "ldr    r1, =%[hwirq]       \n\t"   // load address of HwContext to r1
        "ldm    r0, { r3-r10 }      \n\t"   // use r3-10 as copy buf for HwContext
        "stm    r1, { r3-r10 }      \n\t"   // copy to trusted HwContext

        "ldr    r3, [r0, #24]       \n\t"   // load r3 with the stacked PC
        "ldrb   r0, [r3, #-2]       \n\t"   // extract the imm8 from the instruction into r0
        "push   { lr }              \n\t"
        "bl     %[handler]          \n\t"   // and pass imm8 to SvmRuntime::svc

        "ldr    r0, =%[savedSp]     \n\t"   // copy saved user sp to psp
        "ldr    r0, [r0]            \n\t"
        "msr    psp, r0             \n\t"

        "ldr    r1, =%[hwirq]       \n\t"   // load address of HwContext to r1
        "ldm    r1, { r3-r10 }      \n\t"   // use r3-10 as copy buf for HwContext
        "stm    r0, { r3-r10 }      \n\t"   // copy to newly modified psp

        "ldr    r2, =%[usrirq]      \n\t"   // load pointer to userRegs.irq into r2
        "ldm    r2, { r4-r11 }      \n\t"   // restore user regs

        "pop    { pc }"
        :
        : [hwirq] "i"(&SvmCpu::userRegs.hw),
            [usrirq] "i"(&SvmCpu::userRegs.irq),
            [handler] "i"(SvmRuntime::svc),
            [savedSp] "i"(&SvmCpu::userRegs.sp)
    );
}
