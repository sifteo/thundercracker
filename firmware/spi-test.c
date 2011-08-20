/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Simple test for the on-board SPI controller, in interrupt-driven mode.
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

__xdata uint8_t irq_count;

void rfspi_isr(void) __interrupt (VECTOR_RFSPI)
{
  SPIRDAT = 0x55;

  irq_count++;
  IR_RFSPI = 0;
}

void main(void)
{
  IEN_EN = 1;
  IEN_RFSPI = 1;
  SPIRCON1 = 0xF - SPI_TX_READY;
}
