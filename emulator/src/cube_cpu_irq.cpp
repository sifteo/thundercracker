/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "cube_cpu.h"

namespace Cube {
namespace CPU {


/*
 * Try to invoke an interrupt, of a specified level, with the given
 * IRQ vector. If we can begin the IRQ now, returns nonzero.
 */
int irq_invoke(struct em8051 *cpu, uint8_t priority, uint16_t vector)
{
    if (cpu->isTracing)
        fprintf(cpu->traceFile, "[%2d] IRQ: Interrupt arrived, prio=%d vector=0x%04x\n",
                cpu->id, priority, vector);

    if (cpu->irq_count) {
        // Another IRQ is in progress. Are we higher priority?

        if (priority <= cpu->irql[cpu->irq_count - 1].priority) {
            // Nope. Can't interrupt the last handler.
     
            if (cpu->isTracing)
                fprintf(cpu->traceFile, "[%2d] IRQ: Higher priority interrupt in progress\n", cpu->id);

            return 0;
        }
    }
    
    // Preempt any cycles left on the current SBT block
    cpu->irql[cpu->irq_count].tickDelay = cpu->mTickDelay;
    
    // XXX: A guess at the interrupt latency (LCALL duration)
    cpu->mTickDelay = 6;

    /*
     * Far CALL to the interrupt vector
     */
    cpu->mData[++cpu->mSFR[REG_SP]] = cpu->mPC & 0xff;
    if (cpu->mSFR[REG_SP] == 0) except(cpu, EXCEPTION_STACK);
    cpu->mData[++cpu->mSFR[REG_SP]] = cpu->mPC >> 8;
    if (cpu->mSFR[REG_SP] == 0) except(cpu, EXCEPTION_STACK);
    cpu->mPC = vector;

    /*
     * Remember the priority of this nested handler
     */
    cpu->irql[cpu->irq_count].priority = priority;

    /*
     * Save registers on entry, for optional debug checks.
     */
    cpu->irql[cpu->irq_count].a = cpu->mSFR[REG_ACC];
    cpu->irql[cpu->irq_count].psw = cpu->mSFR[REG_PSW];
    cpu->irql[cpu->irq_count].sp = cpu->mSFR[REG_SP];
    cpu->irql[cpu->irq_count].dpl = cpu->mSFR[REG_DPL];
    cpu->irql[cpu->irq_count].dph = cpu->mSFR[REG_DPH];
    cpu->irql[cpu->irq_count].dpl1 = cpu->mSFR[REG_DPL1];
    cpu->irql[cpu->irq_count].dph1 = cpu->mSFR[REG_DPH1];
    cpu->irql[cpu->irq_count].dps = cpu->mSFR[REG_DPS];
    memcpy(cpu->irql[cpu->irq_count].r, cpu->mSFR, 8);

    cpu->irq_count++;
    return 1;
}

void irq_check(em8051 *aCPU, int i)
{ 
    /*
     * State restore sanity-check
     */

    int psw_bits = PSWMASK_OV | PSWMASK_RS0 | PSWMASK_RS1 | PSWMASK_AC | PSWMASK_C;

    if (UNLIKELY(aCPU->irql[i].a != aCPU->mSFR[REG_ACC]))
        except(aCPU, EXCEPTION_IRET_ACC_MISMATCH);

    if (UNLIKELY(aCPU->irql[i].sp != aCPU->mSFR[REG_SP]))
        except(aCPU, EXCEPTION_IRET_SP_MISMATCH);    

    if (UNLIKELY((aCPU->irql[i].psw & psw_bits) != (aCPU->mSFR[REG_PSW] & psw_bits)))
        except(aCPU, EXCEPTION_IRET_PSW_MISMATCH);

    if (UNLIKELY(aCPU->irql[i].dpl != aCPU->mSFR[REG_DPL] ||
                 aCPU->irql[i].dph != aCPU->mSFR[REG_DPH] ||
                 aCPU->irql[i].dpl1 != aCPU->mSFR[REG_DPL1] ||
                 aCPU->irql[i].dph1 != aCPU->mSFR[REG_DPH1] ||
                 aCPU->irql[i].dps != aCPU->mSFR[REG_DPS]))
        except(aCPU, EXCEPTION_IRET_DP_MISMATCH);    

    if (UNLIKELY(memcmp(aCPU->irql[i].r, aCPU->mSFR, 8)))
        except(aCPU, EXCEPTION_IRET_R_MISMATCH);
}


};  // namespace CPU
};  // namespace Cube
