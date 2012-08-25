/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _UI_ASSETS_H
#define _UI_ASSETS_H

#include "macros.h"
#include <sifteo/abi.h>


/**
 * Information about versioned assets used by system UI.
 *
 * We may have to use different system menu assets depending on the
 * version of the cube's Tile ROM. This object stores all of the
 * version-specific details we need for the individual UIs.
 */

struct UIAssets {

    void init(unsigned version);

    union {
        const uint16_t *images[1];
        struct {
            const uint16_t *logoWhiteOnBlue;
            const uint16_t *menuBackground;
            const uint16_t *iconQuit;
            const uint16_t *iconBack;
            const uint16_t *iconResume;
            const uint16_t *shutdownBackground;
            const uint16_t *bigDigits;
            const uint16_t *faultMessage;
        };
    };

    int8_t menuHeight;
    uint8_t menuTextPalette;
    int8_t iconSize;
    int8_t iconSpacing;
    int8_t shutdownHeight;
    int8_t shutdownY1;
    int8_t shutdownY2;
};

// Calculate an index into images[]
#define UI_IMAGE_INDEX(_name)    ((offsetof(UIAssets, _name) - offsetof(UIAssets, images)) / sizeof(uint16_t*))

#endif
