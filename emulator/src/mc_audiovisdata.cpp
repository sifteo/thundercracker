/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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

#include "mc_audiovisdata.h"


MCAudioVisData MCAudioVisData::instance;


MCAudioVisScope::MCAudioVisScope()
{
    // Fill with silence
    for (unsigned i = 0; i < NUM_SAMPLES * 2; ++i)
        write(0);
}

uint16_t *MCAudioVisScope::getSweep()
{
    /*
     * Return a pointer to SWEEP_LEN contiguous samples, representing a
     * single left-to-right oscilloscope sweep cycle.
     *
     * We have two responsibilities here:
     *
     *    - Our samples are double-buffered. First, choose the logical
     *      buffer opposite from the one where we're currently writing.
     *
     *    - Within that buffer, see if we can find a good trigger position.
     *      This feature helps to horizontally stabilize the display, and it
     *      works much like the trigger functionality on a real oscope.
     *      We do a limited-duration search, starting at the beginning of the
     *      buffer, for a zero crossing.
     */

    uint16_t *buffer = (head >= NUM_SAMPLES) ? &samples[0] : &samples[NUM_SAMPLES];

    STATIC_ASSERT(TRIGGER_SEARCH_LEN < NUM_SAMPLES);
    const uint16_t zero = 0x8000;

    for (unsigned i = 0; i < TRIGGER_SEARCH_LEN; ++i)
        if (buffer[i+1] > zero && buffer[i] < zero)
            return &buffer[i];

    return buffer;
}
