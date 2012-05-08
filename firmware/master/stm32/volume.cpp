
#include "volume.h"
#include "rctimer.h"
#include "board.h"

static RCTimer timer(HwTimer(&VOLUME_TIM), VOLUME_CHAN, VOLUME_GPIO);

namespace Volume {

void init()
{
    timer.init();
}

int systemVolume()
{
    return timer.lastReading();
}

void beginSample()
{
    timer.startSample();
}

} // namespace Volume


IRQ_HANDLER ISR_TIM5()
{
    timer.isr();
    VOLUME_TIM.SR = 0; // must clear SR to ack irq
}
