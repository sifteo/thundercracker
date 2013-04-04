/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _VOLUME_H
#define _VOLUME_H

#include <stdint.h>

namespace Volume
{
    enum CalibrationState {
        CalibrationLow,
        CalibrationHigh
    };

    void init();
    int calibrate(CalibrationState state);
    void beginCapture();
    static void adcCallback(uint16_t sample);

    /*
     * Current system volume.
     *
     * Unlike the individual mixer volumes, this is a 16-bit value
     * in order to give us enough dynamic range to be very very quiet
     * without dropping out completely.
     *
     * Also note that the 'max' volume is actually higher than unity
     * gain. The maximum mixer gain is defined below.
     */
    int systemVolume();

    static const int MAX_VOLUME = 0x10000;
    static const int MAX_VOLUME_LOG2 = 16;

    /*
     * Maximum mixer gain. This is the base-2 logarithm of the gain
     * which corresponds with a "maximum" volume. If this is zero, the
     * maximum volume is 0 dB. Larger values will increase the maximum
     * loudness (with a risk of hitting the limiter). Negative values
     * would have the opposite effect, reducing the maximum loudness.
     */
    static const int MIXER_GAIN_LOG2 = 2;
}

#endif // _VOLUME_H
