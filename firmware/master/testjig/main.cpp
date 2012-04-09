/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usart.h"
#include "board.h"

/*
 * Application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{
    // This is the earliest point at which it's safe to use Usart::Dbg.
    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);

}
