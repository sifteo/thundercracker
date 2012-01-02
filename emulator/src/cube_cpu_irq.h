/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_CPU_IRQ_H
#define _CUBE_CPU_IRQ_H

#include "cube_cpu.h"

namespace Cube {
namespace CPU {

int irq_invoke(struct em8051 *cpu, uint8_t priority, uint16_t vector);
void irq_check(em8051 *aCPU, int i);
void handle_interrupts(struct em8051 *cpu);

};  // namespace CPU
};  // namespace Cube

#endif
