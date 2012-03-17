/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_METADATA_H
#define _SIFTEO_METADATA_H

#include <sifteo/abi.h>

namespace Sifteo {
namespace Metadata {


/**
 * Every game needs a title string.
 * This also sets up some other required metadata. Call it exactly once.
 */

void inline title(const char *t)
{
    uint8_t numAGSlots = _SYS_lti_counter("Sifteo.AssetGroupSlot", -1);
    
    _SYS_lti_metadata(_SYS_METADATA_TITLE_STR, "sB", t, 0);
    _SYS_lti_metadata(_SYS_METADATA_NUM_AGSLOTS, "b", numAGSlots);
}


}   // namespace Metadata
}   // namespace Sifteo

#endif
