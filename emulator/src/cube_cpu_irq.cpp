/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "cube_cpu.h"
#include "cube_cpu_irq.h"

namespace Cube {
namespace CPU {


void NEVER_INLINE handle_interrupts(struct em8051 *cpu)
{
    /*
     * Try to invoke at most one interrupt, starting with the highest
     * priority IRQs.
     *
     * This is _not_ inlined, since we now only check for IRQs when we think
     * they might have changed state, not every clock tick. This means we now want
     * to keep this code out of the main loop, so the whole main loop can fit
     * in the L1 cache.
     */
     
    if (cpu->isTracing)
        fprintf(cpu->traceFile, "[%2d] IRQ: Checking for interrupts\n", cpu->id);

    // IFP is currently disabled, we don't use it.
#ifdef EM8051_SUPPORT_IFP

    /*
     * External interrupts: GPIOs. Only one pin can be selected
     * for use as an Interrupt From Pin (IFP) source at a time.
     */

    uint8_t nextIFP;

    switch (cpu->mSFR[REG_INTEXP] & 0x38) {
    case 0x08:  nextIFP = cpu->mSFR[REG_P1] & (1 << 2);  break;   // GPINT0
    case 0x10:  nextIFP = cpu->mSFR[REG_P1] & (1 << 3);  break;   // GPINT1
    case 0x20:  nextIFP = cpu->mSFR[REG_P1] & (1 << 4);  break;   // GPINT2
    default:    nextIFP = 0;
    };

    if (cpu->mSFR[REG_TCON] & TCONMASK_IT0) {
        // Falling edge
        if (!nextIFP && cpu->ifp)
            cpu->mSFR[REG_TCON] |= TCONMASK_IE0;
    } else {
        // Low level
        if (!nextIFP)
            cpu->mSFR[REG_TCON] |= TCONMASK_IE0;
    }
    cpu->ifp = nextIFP;

#endif  // EM8051_SUPPORT_IFP

    /*
     * Table-driven implementation of the nRF24LE1's IRQ logic. See
     * Figure 46 in section 9.2 of the nRF24LE1 Product Specification.
     *
     * Well, conceptually it's table-driven. In implementation, we
     * unroll it at compile-time using some ugly macros. This saves a
     * lot of time on out-of-order fetches.
     */

#define IRQ_LUT_ENTRY(irqn, ien_sfr, ien_mask, req_sfr, req_mask, autoclear)            \
    {                                                                                   \
        if (UNLIKELY( (cpu->mSFR[req_sfr] & (req_mask)) &&                              \
                      (cpu->mSFR[ien_sfr] & (ien_mask)) )) {                            \
            int prio = ( (!!(cpu->mSFR[REG_IP1] & (ien_mask)) << 1) |                   \
                         (!!(cpu->mSFR[REG_IP0] & (ien_mask)) << 0) );                  \
            if (prio > found_prio) {                                                    \
                found_req_sfr = req_sfr;                                                \
                found_clear_mask = (uint8_t) ~(autoclear);                              \
                found_vector = (irqn) * 8 + 3;                                          \
                found_prio = prio;                                                      \
            }                                                                           \
        }                                                                               \
    }

    // Global IRQ enable
    if (LIKELY(cpu->mSFR[REG_IEN0] & IRQM0_EN)) {

        int found_prio = -1;
        unsigned found_vector;
        uint8_t found_req_sfr;
        uint8_t found_clear_mask;

        IRQ_LUT_ENTRY( 0,  REG_IEN0, IRQM0_IFP,   REG_TCON,  TCONMASK_IE0, TCONMASK_IE0 );
        IRQ_LUT_ENTRY( 8,  REG_IEN1, IRQM1_RFSPI, REG_IRCON, IRCON_RFSPI, 0 );
        IRQ_LUT_ENTRY( 1,  REG_IEN0, IRQM0_TF0,   REG_TCON,  TCONMASK_TF0, TCONMASK_TF0 );
        IRQ_LUT_ENTRY( 9,  REG_IEN1, IRQM1_RF,    REG_IRCON, IRCON_RF, IRCON_RF );
        IRQ_LUT_ENTRY( 2,  REG_IEN0, IRQM0_PFAIL, REG_TCON,  TCONMASK_IE1, TCONMASK_IE1 );
        IRQ_LUT_ENTRY( 10, REG_IEN1, IRQM1_SPI,   REG_IRCON, IRCON_SPI, IRCON_SPI );
        IRQ_LUT_ENTRY( 3,  REG_IEN0, IRQM0_TF1,   REG_TCON,  TCONMASK_TF1, TCONMASK_TF1 );
        IRQ_LUT_ENTRY( 11, REG_IEN1, IRQM1_WUOP,  REG_IRCON, IRCON_WUOP, IRCON_WUOP );
        IRQ_LUT_ENTRY( 4,  REG_IEN0, IRQM0_SER,   REG_S0CON, 0x03, 0 );
        IRQ_LUT_ENTRY( 12, REG_IEN1, IRQM1_MISC,  REG_IRCON, IRCON_MISC, IRCON_MISC );
        IRQ_LUT_ENTRY( 5,  REG_IEN0, IRQM0_TF2,   REG_IRCON, IRCON_TF2 | IRCON_EXF2, 0 );
        IRQ_LUT_ENTRY( 13, REG_IEN1, IRQM1_TICK,  REG_IRCON, IRCON_TICK, IRCON_TICK );

        if (UNLIKELY(found_prio >= 0))
            if (irq_invoke(cpu, found_prio, found_vector)) {
                // Auto-clear if needed
                cpu->mSFR[found_req_sfr] &= found_clear_mask;
            }
    }
}


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
