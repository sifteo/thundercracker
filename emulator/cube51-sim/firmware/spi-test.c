/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Simple test for the on-board SPI controller, in interrupt-driven mode.
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

void rf_reg_write(uint8_t reg, uint8_t value)
{
    RF_CSN = 0;

    SPIRDAT = RF_CMD_W_REGISTER | reg;
    SPIRDAT = value;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;

    RF_CSN = 1;
}
    
void main(void)
{
  RF_CKEN = 1;
  RF_CE = 1;

  while (1) {
      rf_reg_write(RF_REG_CONFIG, 0);
      rf_reg_write(RF_REG_CONFIG, 1);
  }

}
