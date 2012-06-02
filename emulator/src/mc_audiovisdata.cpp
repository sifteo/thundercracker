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
