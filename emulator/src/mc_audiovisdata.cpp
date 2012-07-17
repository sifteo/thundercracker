/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
