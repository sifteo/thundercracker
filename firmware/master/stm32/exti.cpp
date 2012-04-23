#include "vectors.h"

#include "nrf24l01.h"
#include "powermanager.h"

#include "macros.h"

/*
 * Some EXTI vectors serve more than one EXTI line - this serves as a neutral
 * spot to dispatch the pending interrupts.
 */


IRQ_HANDLER ISR_EXTI9_5()
{
#if (BOARD >= BOARD_TC_MASTER_REV1)

    if (NRF24L01::instance.irq.irqPending())
        NRF24L01::instance.isr();

    if (PowerManager::vbus.irqPending())
        PowerManager::vbusIsr();

#endif
}
