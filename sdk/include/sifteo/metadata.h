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
     * Initialize all required system metadata. Other optional metadata can be
     * added using individual methods on the Metadata class.
     */

    Metadata()
    {
        // Metadata that's automatically inserted only once
        if (_SYS_lti_counter("Sifteo.Metadata", 0) == 0) {

            // Count the total number of AssetGroupSlots in use
            unsigned numAGSlots = _SYS_lti_counter("Sifteo.AssetGroupSlot", -1);
            _SYS_lti_metadata(_SYS_METADATA_NUM_AGSLOTS, "b", numAGSlots);

            // UUID for this particular build. Used by the system for asset caching.
            _SYS_lti_metadata(_SYS_METADATA_UUID, "IIII",
                _SYS_lti_uuid(0, 0), _SYS_lti_uuid(0, 1),
                _SYS_lti_uuid(0, 2), _SYS_lti_uuid(0, 3));
        }
    }
    
    Metadata &title(const char *str)
    {
        _SYS_lti_abort(_SYS_lti_counter("Sifteo.Metadata.Title", 0) != 0,
            "Duplicate Metadata::title() instance.");

        _SYS_lti_metadata(_SYS_METADATA_TITLE_STR, "sB", str, 0);

        return *this;
    }
    
    Metadata &icon(const PinnedAssetImage &image)
    {
        STATIC_ASSERT(image.width == 10);
        STATIC_ASSERT(image.height == 10);
        _SYS_lti_abort(_SYS_lti_counter("Sifteo.Metadata.Icon", 0) != 0,
            "Duplicate Metadata::icon() instance.");
        
        AssetGroup *G = (AssetGroup*) _SYS_lti_initializer(image.group);
        _SYS_lti_metadata(_SYS_METADATA_ICON_80x80, "IHH", G->sys.pHdr, image.index, 0);

        return *this;
    }
};

}   // namespace Sifteo

#endif
