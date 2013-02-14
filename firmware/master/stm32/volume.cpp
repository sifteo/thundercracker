
#include "volume.h"
#include "rctimer.h"
#include "board.h"
#include "macros.h"
#include "systime.h"
#include "tasks.h"
#include "audiooutdevice.h"
#include <sifteo/abi/audio.h>

static volatile bool audioOff;
static volatile SysTime::Ticks pwmTimeStamp;

namespace Volume {

void init()
{
    audioOff = false;
    pwmTimeStamp = 0;
    
    GPIOPin volumeSlider = VOLUME_GPIO;
    volumeSlider.setControl(GPIOPin::IN_FLOAT);
    volumeSlider.irqInit();
    volumeSlider.irqSetRisingEdge();
    volumeSlider.irqEnable();
}

int systemVolume()
{
    if(audioOff) {
        Tasks::trigger(Tasks::Volume);
        return 0;
    } else {
        return MAX_VOLUME;
    }
}

int calibrate(CalibrationState state)
{
    if(audioOff) {
        return 0;
    } else {
        return MAX_VOLUME;
    }
}

void updateTimeStamp()
{
    pwmTimeStamp = SysTime::ticks();
}

bool isActive()
{
    return VOLUME_GPIO.isLow();
}

void task()
{
    AudioOutDevice::setLow();

    if ( isActive() ) {
        AudioOutDevice::setHigh();
        audioOff = false;
    }
}

} // namespace Volume

#if (BOARD != BOARD_TEST_JIG)
IRQ_HANDLER ISR_EXTI1()
{
    VOLUME_GPIO.irqAcknowledge();
    audioOff = true;
    Volume::updateTimeStamp();

}
#endif
