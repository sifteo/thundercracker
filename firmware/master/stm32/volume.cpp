
#include "volume.h"
#include "rctimer.h"
#include "board.h"
#include "macros.h"
#include <sifteo/abi/audio.h>

static RCTimer gVolumeTimer(HwTimer(&VOLUME_TIM), VOLUME_CHAN, VOLUME_GPIO);

static uint32_t calibratedMin;
static uint64_t calibratedScale;


namespace Volume {

void init()
{
    /*
     * Defaults in case stored calibration data is not available.
     *
     * I have observed the min to be around 60, and max to be around 640.
     * Tightening the window a bit to make sure we can get all the way to the
     * extremes, even when accommodating some variation.
     *
     * These values are in arbitrary units, as returned by RCTimer::lastReading().
     */
    const int DefaultMin = 100 << 8;
    const int DefaultMax = 600 << 8;

    /*
     * TODO: read calibration readings taken at factory test time
     * and stored in external flash.
     *
     * Fall back to defaults for now.
     */
    calibratedMin = DefaultMin;
    int calibratedMax = DefaultMax;

    /*
     * Calculate a scale factor from raw units to volume units, in 32.32 fixed point.
     */
    calibratedScale = (uint64_t(MAX_VOLUME) << 32) / (calibratedMax - calibratedMin);

    /*
     * Specify the rate at which we'd like to sample the volume level.
     * We want to balance between something that's not frequent enough,
     * and something that consumes too much power.
     *
     * The prescaler also affects the precision with which the timer will
     * capture readings - we don't need anything too detailed, so we can
     * dial it down a bit.
     */
    gVolumeTimer.init(0xfff, 6);
}

int systemVolume()
{
    int reading = gVolumeTimer.lastReading();
    int scaled = ((reading - calibratedMin) * calibratedScale + 0x80000000) >> 32;
    return clamp<int>(scaled, 0, MAX_VOLUME);
}

int calibrate(CalibrationState state)
{
    int currentRawValue = gVolumeTimer.lastReading();

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
    gVolumeTimer.isr();    // must clear the TIM IRQ internally
}
#endif
