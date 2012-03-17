/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmcpu.h"
#include "svmruntime.h"

#include "vectors.h"

using namespace Svm;

namespace SvmCpu {

static struct  {
    IrqContext irq;
    HwContext hw;
    reg_t sp;
} userRegs;


/*
 * These routines are temp - this should be much better, but it friggin works, so checking in!
 */

static void saveHwContext(reg_t sp)
{
    uint32_t *spp = reinterpret_cast<uint32_t*>(sp);
    uint32_t *hw = reinterpret_cast<uint32_t*>(&userRegs.hw);
    userRegs.sp = sp;
    for (unsigned i = 0; i < sizeof userRegs.hw / 4; ++i) {
        hw[i] = spp[i];
    }
//    memcpy(&userRegs.hw, spp, sizeof userRegs.hw);
}

static void restoreHwContext()
{
    uint32_t *spp = reinterpret_cast<uint32_t*>(userRegs.sp);
    uint32_t *hw = reinterpret_cast<uint32_t*>(&userRegs.hw);
    for (unsigned i = 0; i < sizeof userRegs.hw / 4; ++i) {
        spp[i] = hw[i];
    }
//    memcpy(spp, &userRegs.hw, sizeof userRegs.hw);
}

/***************************************************************************
 * Public Functions
 ***************************************************************************/

void init()
{
    memset(&userRegs, 0, sizeof userRegs);
}

/*
 * Run an application.
 *
 * Set the user stack pointer to the provided value,
 * and write the CONTROL special register to both switch to user mode,
 * and start using the user mode stack.
 *
 * Finally, branch into user code.
 */
void run(reg_t sp, reg_t pc)
{
    asm volatile(
        "msr    psp, %[sp_arg]          \n\t"
        "mov    r10, %[target]          \n\t"
        "mov    r0, #0                  \n\t"
        "mov    r1, #0                  \n\t"
        "mov    r2, #0                  \n\t"
        "mov    r3, #0                  \n\t"
        "mov    r4, #0                  \n\t"
        "mov    r5, #0                  \n\t"
        "mov    r6, #0                  \n\t"
        "mov    r7, #0                  \n\t"
        "mov    r8, #0                  \n\t"
        "mov    r9, #0                  \n\t"
        "mov    r12, #0x3               \n\t"
        "msr    control, r12            \n\t"
        "isb                            \n\t"
        "bx     r10"
        :
        : [sp_arg] "r"(sp), [target] "r"(pc)
    );
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
    default:        return 0;   // TODO: trap this?
    }
}

void setReg(uint8_t r, reg_t val)
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
    // trap default case as invalid?
    }
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
        "push   { lr }              \n\t"
        "bl     %[saveHW]           \n\t"   // save stacked hw regs (sp is in r0) - can optimize to asm when needed
        "ldr    r2, =%[usrirq]      \n\t"   // load pointer to userRegs.irq into r2
        "stm    r2, { r4, r5, r6, r7, r8, r9, r10, r11 }    \n\t"   // save copy of user regs
        "ldr    r3, [r0, #24]       \n\t"   // load r3 with the stacked PC
        "ldrb   r0, [r3, #-2]       \n\t"   // extract the imm8 from the instruction into r0
        "bl     %[target]           \n\t"   // and pass sp and imm8 to SvmRuntime::svc
        "ldr    r2, =%[usrirq]      \n\t"   // load pointer to userRegs.irq into r2
        "ldm    r2, { r4, r5, r6, r7, r8, r9, r10, r11 }    \n\t"   // restore saved regs
        "ldr    r3, =%[savedSp]     \n\t"
        "ldr    r3, [r3]            \n\t"
        "msr    psp, r3             \n\t"   // copy saved user sp to psp
        "bl     %[restoreHW]        \n\t"   // copy hw regs to potentially modified user sp
        "pop    { pc }"
        :
        : [saveHW] "i"(SvmCpu::saveHwContext),
            [usrirq] "i"(&SvmCpu::userRegs.irq),
            [target] "i"(SvmRuntime::svc),
            [restoreHW] "i"(SvmCpu::restoreHwContext),
            [savedSp] "i"(&SvmCpu::userRegs.sp)
    );
}
