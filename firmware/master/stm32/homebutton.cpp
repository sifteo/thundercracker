/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "homebutton.h"
#include "gpio.h"
#include "board.h"
#include "tasks.h"
#include "pause.h"

namespace HomeButton {

void init()
{

    sampleTicksPointer = 0;

    for(int i = 0; i < 100; i++) {
        samplePolarity[i] = 0;
        sampleTicks[i] = 0;
    }

    GPIOPin homeButton = BTN_HOME_GPIO;
    homeButton.setControl(GPIOPin::IN_FLOAT);
    homeButton.irqInit();
    homeButton.irqSetRisingEdge();
    homeButton.irqSetFallingEdge();
    homeButton.irqEnable();
}

bool isPressed()
{
    return BTN_HOME_GPIO.isHigh();
}

} // namespace HomeButton


#if (BOARD >= BOARD_TC_MASTER_REV2)
IRQ_HANDLER ISR_EXTI2()
{
    if(sampleTicksPointer < 100) {
        sampleTicks[sampleTicksPointer] = SysTime::ticks();
        
        if(BTN_HOME_GPIO.isHigh()){
            samplePolarity[sampleTicksPointer] = 1;
        } else {
            samplePolarity[sampleTicksPointer] = 0;
        }
            
        sampleTicksPointer++;
    }

    BTN_HOME_GPIO.irqAcknowledge();
    HomeButton::update();
    Pause::taskWork.atomicMark(Pause::ButtonPress);
    Tasks::trigger(Tasks::Pause);
}
#endif

