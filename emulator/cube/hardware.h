/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "emu8051.h"

void hardware_init(struct em8051 *cpu);
void hardware_exit(void);

void hardware_sfrwrite(struct em8051 *cpu, int reg);
int hardware_sfrread(struct em8051 *cpu, int reg);
void hardware_tick(struct em8051 *cpu);

#endif
