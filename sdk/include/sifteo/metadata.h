/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_METADATA_H
#define _SIFTEO_METADATA_H

#include <sifteo/abi.h>

namespace Sifteo {

class Metadata {
public:
    /**
     * Initialize all required metadata. Other optional metadata can be
     * added using individual methods on the Metadata class.
     */

    Metadata(const char *gameTitle)
    {
        unsigned numAGSlots = _SYS_lti_counter("Sifteo.AssetGroupSlot", -1);
    
        _SYS_lti_metadata(_SYS_METADATA_TITLE_STR, "sB", gameTitle, 0);
        _SYS_lti_metadata(_SYS_METADATA_NUM_AGSLOTS, "b", numAGSlots);

        _SYS_lti_abort(_SYS_lti_counter("Sifteo.Metadata", 0) != 0,
            "Only one instance of Sifteo::Metadata is allowed!");
    }
};

}   // namespace Sifteo

#endif
