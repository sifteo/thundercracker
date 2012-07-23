/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "homebutton.h"
#include "gpio.h"
#include "board.h"
#include "powermanager.h"
#include "tasks.h"
#include "systime.h"

static GPIOPin homeButton = BTN_HOME_GPIO;
static SysTime::Ticks shutdownDeadline;

namespace HomeButton {

void init()
{
    GPIOPin green = LED_GREEN_GPIO;
    green.setHigh();
    green.setControl(GPIOPin::OUT_2MHZ);

    homeButton.setControl(GPIOPin::IN_FLOAT);
    homeButton.irqInit();
    homeButton.irqSetRisingEdge();
    homeButton.irqEnable();
}

/*
    Called in ISR context when we detect an edge on the home button.
*/
void onChange()
{
    homeButton.irqAcknowledge();

    /*
     * Begin our countdown to shutdown.
     * Turn on the LED to provide some responsiveness.
     */
    shutdownDeadline = SysTime::ticks() + SysTime::sTicks(3);

    GPIOPin green = LED_GREEN_GPIO;
    green.setLow();

    Tasks::trigger(Tasks::HomeButton);
}

bool isPressed()
{
    return homeButton.isHigh();
}

/*
 * Called repeatedly from within Tasks::work to handle a button event on the main loop.
 *
 * Temporary home button handling: power off.
 * Wait for the button to be held long enough to be sure it's a shut down request,
 * then blink the green LED to indicate we're going away.
 */
void task()
{
    /*
     * If the button has been released, we're done.
     */
    if (!isPressed()) {
        GPIOPin green = LED_GREEN_GPIO;
        green.setHigh();
        return;
    }
    
    // Do nothing if we're connected to USB power
    if (PowerManager::vbus.isHigh()) 
        return;

    // Keep polling while the button is held down
    if (SysTime::ticks() < shutdownDeadline) {
        Tasks::trigger(Tasks::HomeButton);
        return;
    }

    // power off sequence
    GPIOPin green = LED_GREEN_GPIO;

    for (volatile unsigned blinks = 0; blinks < 10; ++blinks) {
        for (volatile unsigned count = 0; count < 1000000; ++count) {
            ;
        }
        green.toggle();
    }

    PowerManager::shutdown();
    // goodbye, cruel world
    for (;;)
        ;
}

} // namespace Button

#if (BOARD == BOARD_TC_MASTER_REV1)
IRQ_HANDLER ISR_EXTI0()
{
    Button::isr();
}
#elif (BOARD >= BOARD_TC_MASTER_REV2)
IRQ_HANDLER ISR_EXTI2()
{
    HomeButton::onChange();
}
#elif (BOARD == BOARD_TEST_JIG)
// this isr is used elsewhere for the test jig
#else
#error "no button isr declared";
#endif
