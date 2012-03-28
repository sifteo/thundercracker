
#include "button.h"
#include "board.h"

GPIOPin Button::homeButton = BTN_HOME_GPIO;

void Button::init()
{
    homeButton.setControl(GPIOPin::IN_FLOAT);
    homeButton.irqInit();
    homeButton.irqSetRisingEdge();
    homeButton.irqEnable();
}

/*
    Temporary home button handling: power off.
    When we get a button edge, wait for the button to be held long enough
    to be sure it's a shut down request, then blink the green LED to indicate
    we're going away.
*/
void Button::isr()
{
    homeButton.irqAcknowledge();

    GPIOPin green = LED_GREEN_GPIO;
    green.setControl(GPIOPin::OUT_10MHZ);
    green.setLow();

    // these durations are totally ad hoc.
    // not using SysTime such that we don't need to worry about this ISR being
    // higher priority than SysTick, in which case the clock might not progress.

    // ensure we're held high long enough before turning off
    for (volatile unsigned dur = 0; dur < 2000000; ++dur) {
        if (!homeButton.isHigh()) {
            green.setHigh();
            return;
        }
    }

    // power off sequence
    for (volatile unsigned blinks = 0; blinks < 10; ++blinks) {
        for (volatile unsigned count = 0; count < 1000000; ++count) {
            ;
        }
        green.toggle();
    }

    // release the power supply enable
    GPIOPin vcc20 = VCC20_ENABLE_GPIO;
    vcc20.setControl(GPIOPin::OUT_2MHZ);
    vcc20.setLow();
}

#if (BOARD == BOARD_TC_MASTER_REV1)
IRQ_HANDLER ISR_EXTI0()
{
    Button::isr();
}
#elif (BOARD == BOARD_TC_MASTER_REV2)
IRQ_HANDLER ISR_EXTI2()
{
    Button::isr();
}
#else
#error "no button isr declared";
#endif
