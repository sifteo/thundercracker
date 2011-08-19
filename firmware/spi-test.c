/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Simple test for the on-board SPI controller, in interrupt-driven mode.
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

__xdata uint8_t c;

void rfspi_isr(void) __interrupt (VECTOR_RFSPI)
{
  c++;
  IR_RFSPI = 0;
}

void main(void)
{
  IEN_EN = 1;
  IEN_RFSPI = 1;

  IR_RFSPI = 1;
}
