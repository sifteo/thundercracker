/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF24L01 radio
 */

#include "nrf51822.h"

SWDMaster nrf51822::btle_swd(&BTLE_SWD_TIM, BTLE_SWD_CLK_GPIO, BTLE_SWD_IO_GPIO);

void nrf51822::test()
{
    btle_swd.init();
    btle_swd.enableRequest();
    //while(!btle_swd.readRequest(0xa5)); //0b10100101)
}

IRQ_HANDLER ISR_FN(BTLE_SWD_TIM_INT)()
{
    nrf51822::btle_swd.isr();
}
