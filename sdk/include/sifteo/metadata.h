/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_METADATA_H
#define _SIFTEO_METADATA_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

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

    /**
     * Add a human-readable title string to this game's metadata.
     */
    Metadata &title(const char *str)
    {
        _SYS_lti_abort(_SYS_lti_counter("Sifteo.Metadata.Title", 0) != 0,
            "Duplicate Metadata::title() instance.");

        _SYS_lti_metadata(_SYS_METADATA_TITLE_STR, "sB", str, 0);

        return *this;
    }

    /**
     * Add an icon image to this game's metadata. The image needs to be
     * 80x80 pixels, and it should reside in a separate AssetGroup.
     */
    Metadata &icon(const _SYSAssetImage &i)
    {
        _SYS_lti_abort(_SYS_lti_counter("Sifteo.Metadata.Icon", 0) != 0,
            "Duplicate Metadata::icon() instance.");

        return image(_SYS_METADATA_ICON_80x80, i);
    }

    /**
     * Add an arbitrary image, as a _SYSMetadataImage item, with a
     * user-specified key.
     */
    Metadata &image(uint16_t key, const _SYSAssetImage &i)
    {
        // AssetGroup is in RAM, but we want the static initializer data
        _SYSAssetGroup *G = (_SYSAssetGroup*) _SYS_lti_initializer(
            reinterpret_cast<void*>(i.pAssetGroup));

        // Build a _SYSMetadataImage struct
        _SYS_lti_metadata(key, "BBBBII",
            i.width, i.height, i.frames, i.format, G->pHdr, i.data);

        return *this;
    }
};


}   // namespace Sifteo

#endif
