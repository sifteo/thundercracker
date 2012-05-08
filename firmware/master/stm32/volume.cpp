
#include "volume.h"
#include "rctimer.h"
#include "board.h"

static RCTimer timer(HwTimer(&VOLUME_TIM), VOLUME_CHAN, VOLUME_GPIO);

namespace Volume {

void init()
{
    /*
     * Specify the rate at which we'd like to sample the volume level.
     * We want to balance between something that's not frequent enough,
     * and something that consumes too much power.
     *
     * The prescaler also affects the precision with which the timer will
     * capture readings - we don't need anything too detailed, so we can
     * dial it down a bit.
     */
    timer.init(0xfff, 3);
}

int systemVolume()
{
    // XXX: scale this to something meaningful
    return timer.lastReading();
}

} // namespace Volume


IRQ_HANDLER ISR_TIM5()
{
    timer.isr();    // must clear the TIM IRQ internally
}
