
#include "volume.h"
#include "rctimer.h"
#include "board.h"
#include "macros.h"
#include <sifteo/abi/audio.h>

static RCTimer gVolumeTimer(HwTimer(&VOLUME_TIM), VOLUME_CHAN, VOLUME_GPIO);

static int calibratedMin;
static int calibratedScale;


namespace Volume {

void init()
{
    /*
     * Defaults in case stored calibration data is not available.
     *
     * I have observed the min to be around 60, and max to be around 640.
     * Tightening the window a bit to make sure we can get all the way to the
     * extremes, even when accommodating some variation.
     */
    const int DefaultMin = 100;
    const int DefaultMax = 600;

    /*
     * TODO: read calibration readings taken at factory test time
     * and stored in external flash.
     *
     * Fall back to defaults for now.
     */
    calibratedMin = DefaultMin;
    int calibratedMax = DefaultMax;

    /*
     * Calculate a scale factor from raw units to volume units, in 16.16 fixed point.
     */
    calibratedScale = (_SYS_AUDIO_MAX_VOLUME << 16) / (calibratedMax - calibratedMin);

    /*
     * Specify the rate at which we'd like to sample the volume level.
     * We want to balance between something that's not frequent enough,
     * and something that consumes too much power.
     *
     * The prescaler also affects the precision with which the timer will
     * capture readings - we don't need anything too detailed, so we can
     * dial it down a bit.
     */
    gVolumeTimer.init(0xfff, 3);
}

int systemVolume()
{
    int reading = gVolumeTimer.lastReading();
    int scaled = ((reading - calibratedMin) * calibratedScale + 0x8000) >> 16;
    return clamp<int>(scaled, 0, _SYS_AUDIO_MAX_VOLUME);
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
