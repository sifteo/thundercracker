#include "vectors.h"

#include "nrf24l01.h"
#include "powermanager.h"
#include "board.h"
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
#if (BOARD >= BOARD_TC_MASTER_REV1)

    if (NRF24L01::instance.irq.irqPending())
        NRF24L01::instance.isr();

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
