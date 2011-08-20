/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _RADIO_H
#define _RADIO_H

#include <stdint.h>

void radio_init(void);
void radio_exit(void);

uint8_t radio_spi_byte(uint8_t mosi);
void radio_ctrl(int csn, int ce);
int radio_tick(void);

#endif
