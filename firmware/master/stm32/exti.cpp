/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "vectors.h"

#include "board.h"
#include "nrf24l01.h"
#include "nrf8001/nrf8001.h"
#include "powermanager.h"
#include "neighbor_rx.h"

#if (BOARD == BOARD_TEST_JIG)
#include "testjig.h"
#endif

/*
 * Some EXTI vectors serve more than one EXTI line - this serves as a neutral
 * spot to dispatch the pending interrupts.
 */


IRQ_HANDLER ISR_EXTI9_5()
{
#ifdef USE_NRF24L01
    if (NRF24L01::instance.irq.irqPending())
        NRF24L01::instance.isr();
#endif

#if defined(HAVE_NRF8001) && (BOARD == BOARD_TC_MASTER_REV3)
    if (NRF8001::instance.rdyn.irqPending()) {
        NRF8001::instance.isr();
    }
#endif

#if (BOARD >= BOARD_TC_MASTER_REV1 && BOARD != BOARD_TEST_JIG)
    if (PowerManager::vbus.irqPending()) {
        PowerManager::vbus.irqAcknowledge();
        PowerManager::onVBusEdge();
    }
#endif

#if (BOARD == BOARD_TEST_JIG)
    // neighbor ins 0 and 1 are on exti lines 6 and 7 respectively
    NeighborRX::pulseISR(0);
    NeighborRX::pulseISR(1);
#endif
}
