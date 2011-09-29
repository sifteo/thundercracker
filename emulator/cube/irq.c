/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "emu8051.h"

/*
 * Try to invoke an interrupt, of a specified level, with the given
 * IRQ vector. If we can begin the IRQ now, returns nonzero.
 */
static int irq_invoke(struct em8051 *cpu, uint8_t priority, uint16_t vector)
{
    if (cpu->irq_count) {
        // Another IRQ is in progress. Are we higher priority?

        if (priority <= cpu->irql[cpu->irq_count - 1].priority)
            // Nope. Can't interrupt the last handler.
            return 0;
    }

    /*
     * Far CALL to the interrupt vector
     */
    push_to_stack(cpu, cpu->mPC & 0xff);
    push_to_stack(cpu, cpu->mPC >> 8);
    cpu->mPC = vector;
    cpu->mTickDelay += 6;  // Kind of a guess...

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

    cpu->irq_count++;
    return 1;
}


/*
 * Try to invoke at most one interrupt, starting with the highest
 * priority IRQs.
 */
void handle_interrupts(struct em8051 *cpu)
{
    /*
     * Table-driven implementation of the nRF24LE1's IRQ logic.
     * See Figure 46 in section 9.2 of the nRF24LE1 Product Specification.
     */
    static const struct {
        uint16_t irqn;
        uint8_t ien_sfr;
        uint8_t ien_mask;  // Also priority mask
        uint8_t req_sfr;
        uint8_t req_mask;
        uint8_t autoclear;
    } irq_lut[] = {
        { 0,  REG_IEN0, IRQM0_IFP,   REG_TCON,  TCONMASK_IE0, 0 },
        { 8,  REG_IEN1, IRQM1_RFSPI, REG_IRCON, IRCON_RFSPI, 0 },
        { 1,  REG_IEN0, IRQM0_TF0,   REG_TCON,  TCONMASK_TF0, 0 },
        { 9,  REG_IEN1, IRQM1_RF,    REG_IRCON, IRCON_RF, IRCON_RF },
        { 2,  REG_IEN0, IRQM0_PFAIL, REG_TCON,  TCONMASK_IE1, 0 },
        { 10, REG_IEN1, IRQM1_SPI,   REG_IRCON, IRCON_SPI, IRCON_SPI },
        { 3,  REG_IEN0, IRQM0_TF1,   REG_TCON,  TCONMASK_TF1, 0 },
        { 11, REG_IEN1, IRQM1_WUOP,  REG_IRCON, IRCON_WUOP, IRCON_WUOP },
        { 4,  REG_IEN0, IRQM0_SER,   REG_S0CON, 0x03, 0 },
        { 12, REG_IEN1, IRQM1_MISC,  REG_IRCON, IRCON_MISC, IRCON_MISC },
        { 5,  REG_IEN0, IRQM0_TF2,   REG_IRCON, IRCON_TF2 | IRCON_EXF2, 0 },
        { 13, REG_IEN1, IRQM1_TICK,  REG_IRCON, IRCON_TICK, IRCON_TICK },
    };

    int i;
    int found_i = -1;
    int found_prio = -1;

    // Global IRQ enable
    if (!(cpu->mSFR[REG_IEN0] & IRQM0_EN))
        return;

    // Look for the highest priority active interrupt
    for (i = 0; i < (sizeof irq_lut / sizeof irq_lut[0]); i++)
        if ((cpu->mSFR[irq_lut[i].ien_sfr] & irq_lut[i].ien_mask) &&
            (cpu->mSFR[irq_lut[i].req_sfr] & irq_lut[i].req_mask)) {

            int prio = ( (!!(cpu->mSFR[REG_IP1] & irq_lut[i].ien_mask) << 1) |
                         (!!(cpu->mSFR[REG_IP0] & irq_lut[i].ien_mask) << 0) );

            if (prio > found_prio) {
                found_i = i;
                found_prio = prio;
            }
        }

    if (found_i >= 0)
        if (irq_invoke(cpu, found_prio, irq_lut[found_i].irqn * 8 + 3)) {
            // Auto-clear if needed
            cpu->mSFR[irq_lut[found_i].req_sfr] &= ~irq_lut[found_i].autoclear;
        }
}
