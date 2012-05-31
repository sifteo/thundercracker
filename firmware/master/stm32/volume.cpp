
#include "volume.h"
#include "rctimer.h"
#include "board.h"
#include "macros.h"
#include <sifteo/abi/audio.h>

static RCTimer timer(HwTimer(&VOLUME_TIM), VOLUME_CHAN, VOLUME_GPIO);

static uint16_t calibratedMin;
static uint16_t calibratedMax;

namespace Volume {

void init()
{
    /*
     * Defaults in case stored calibration data is not available.
     *
     * I have observed the min to be around 80, and max to be around 875.
     * Tightening the window a bit to make sure we can get all the way to the
     * extremes, even when accommodating some variation.
     */
    const uint16_t DefaultMin = 100;
    const uint16_t DefaultMax = 800;

    /*
     * TODO: read calibration readings taken at factory test time
     * and stored in external flash.
     *
     * Fall back to defaults for now.
     */
    calibratedMin = DefaultMin;
    calibratedMax = DefaultMax;

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

uint16_t systemVolume()
{
    uint16_t reading = clamp(timer.lastReading(), calibratedMin, calibratedMax);
    uint16_t scaledVolume = scale(reading, calibratedMin, calibratedMax,
                                  (uint16_t)0, (uint16_t)_SYS_AUDIO_MAX_VOLUME);
    return scaledVolume;
}

uint16_t calibrate(CalibrationState state)
{
    uint16_t currentRawValue = timer.lastReading();

    /*
     * Save the currentRawValue as the calibrated value for the given state.
     *
     * TODO: determine how we're going to store calibration data and implement me!
     */
    switch (state) {
    case CalibrationLow:
        break;
    case CalibrationHigh:
        break;
    }

    return currentRawValue;
}

} // namespace Volume

#if (BOARD != BOARD_TEST_JIG)
IRQ_HANDLER ISR_TIM5()
{
    timer.isr();    // must clear the TIM IRQ internally
}
#endif
