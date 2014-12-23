/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
