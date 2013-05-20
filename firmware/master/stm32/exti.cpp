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
